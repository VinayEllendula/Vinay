import { queryRedshift } from "@/src/server/redshift"; // adjust import path
// other imports stay the same: FilterList, orderByTimeSeries, etc.

export const getObservationUsageByTypeByTime = async (
  projectId: string,
  filter: FilterState,
) => {
  const { envFilter, remainingFilters } =
    extractEnvironmentFilterFromFilters(filter);

  const environmentFilter = new FilterList(
    convertEnvFilterToClickhouseFilter(envFilter),
  ).apply();

  const chFilter = new FilterList(
    createFilterFromFilterState(remainingFilters, dashboardColumnDefinitions),
  );

  const appliedFilter = chFilter.apply();

  const tracesFilter = chFilter.find((f) => f.clickhouseTable === "traces");

  const timeFilter = tracesFilter
    ? (chFilter.find(
        (f) =>
          f.clickhouseTable === "observations" &&
          f.field.includes("start_time") &&
          (f.operator === ">=" || f.operator === ">"),
      ) as DateTimeFilter | undefined)
    : undefined;

  const [orderByQuery, orderByParams, bucketSizeInSeconds] = orderByTimeSeries(
    filter,
    "start_time",
  );

  // ---------- REDSHIFT QUERY ----------
  // usage_details is SUPER. We use FLATTEN to explode key/value pairs.
  //
  // Inner SELECT:
  //   - bucketed start_time (via selectTimeseriesColumn)
  //   - f.key as usage_key
  //   - CAST(f.value AS DOUBLE PRECISION) as usage
  //
  // Outer SELECT:
  //   - SUM(usage) per (start_time, usage_key)
  //
  // We return one row per (bucket, usage_key) and rebuild the
  // "all keys for all buckets" grid in TS below.
  const query = `
    SELECT
      start_time,
      usage_key,
      SUM(usage) AS usage_sum
    FROM (
      SELECT
        ${selectTimeseriesColumn(
          bucketSizeInSeconds,
          "o.start_time",
          "start_time",
        )},
        f.key AS usage_key,
        CAST(f.value AS DOUBLE PRECISION) AS usage
      FROM
        mmo_schema.observations AS o
      ${tracesFilter
        ? "LEFT JOIN mmo_schema.traces AS t ON o.trace_id = t.id AND o.project_id = t.project_id"
        : ""}
      CROSS JOIN LATERAL FLATTEN(input => o.usage_details) AS f
      WHERE
        o.project_id = ?
        ${appliedFilter.query ? `AND ${appliedFilter.query}` : ""}
        ${environmentFilter.query ? `AND ${environmentFilter.query}` : ""}
        ${
          timeFilter
            ? `AND t.timestamp >= ? - ${OBSERVATIONS_TO_TRACE_INTERVAL}`
            : ""
        }
    ) AS s
    GROUP BY
      start_time,
      usage_key
    ${orderByQuery}
  `;

  const params: any[] = [
    projectId,
    ...appliedFilter.params,
    ...environmentFilter.params,
    ...orderByParams,
  ];

  if (timeFilter) {
    // Redshift driver will accept a JS Date; if you have a helper for this,
    // you can wrap it, e.g. convertDateToRedshiftTimestamp(timeFilter.value).
    params.push(timeFilter.value);
  }

  const result = await queryRedshift<{
    start_time: string;
    usage_key: string;
    usage_sum: number | null;
  }>({
    query,
    params,
    tags: {
      feature: "dashboard",
      type: "observationUsageByTime",
      kind: "analytic",
      projectId,
    },
  });

  // ---------- POST-PROCESSING ----------
  // 1. Collect all unique usage keys.
  // 2. Group rows by start_time → { key: sum }.
  // 3. Produce one row per (intervalStart, key) with sum (0 if missing),
  //    keeping the same return shape as the ClickHouse version.

  const allTypes = new Set<string>();
  const byBucket = new Map<
    string,
    {
      [usageKey: string]: number;
    }
  >();

  for (const row of result) {
    allTypes.add(row.usage_key);

    const bucketKey = row.start_time;
    const existing = byBucket.get(bucketKey) ?? {};
    existing[row.usage_key] = row.usage_sum != null ? Number(row.usage_sum) : 0;
    byBucket.set(bucketKey, existing);
  }

  const uniqueTypes = [...allTypes];
  const finalResult: {
    intervalStart: Date;
    key: string;
    sum: number;
  }[] = [];

  for (const [bucketStart, usageByType] of byBucket.entries()) {
    const intervalStart = parseClickhouseUTCDateTimeFormat(bucketStart); // still OK for ISO strings
    for (const type of uniqueTypes) {
      finalResult.push({
        intervalStart,
        key: type,
        sum: usageByType[type] ?? 0,
      });
    }
  }

  return finalResult;
};
