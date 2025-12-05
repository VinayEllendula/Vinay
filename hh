function convertNamedToPgParams(
  query: string,
  namedParams: Record<string, any>,
  startIndex = 1,
): { query: string; params: any[] } {
  const namesInOrder: string[] = [];

  const newQuery = query.replace(
    /\{([a-zA-Z0-9_]+):[^}]*\}/g,
    (_match, name: string) => {
      let idx = namesInOrder.indexOf(name);
      if (idx === -1) {
        namesInOrder.push(name);
        idx = namesInOrder.length - 1;
      }
      return `$${startIndex + idx}`;
    },
  );

  const params = namesInOrder.map((n) => namedParams[n]);
  return { query: newQuery, params };
}



function redshiftBucketExpr(bucketSizeInSeconds: number, column: string): string {
  // floor(DATEDIFF / bucket) * bucket to get the bucket start in seconds since epoch
  // then add back to '1970-01-01'
  return `DATEADD(
    second,
    FLOOR(DATEDIFF(second, TIMESTAMP '1970-01-01 00:00:00', ${column}) / ${bucketSizeInSeconds}) * ${bucketSizeInSeconds},
    TIMESTAMP '1970-01-01 00:00:00'
  )`;
}


import { queryRedshift } from "@/src/server/redshift";
// other imports stay the same

export const getObservationUsageByTypeByTime = async (
  projectId: string,
  filter: FilterState,
) => {
  const { envFilter, remainingFilters } =
    extractEnvironmentFilterFromFilters(filter);

  // still using the existing filter builders (they return ClickHouse-style named params)
  const environmentFilter = new FilterList(
    convertEnvFilterFromFilters(envFilter),
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

  // We only use the bucket size from this helper; we ignore its ORDER BY / WITH FILL
  const [, , bucketSizeInSeconds] = orderByTimeSeries(filter, "start_time");

  // ---------- RAW QUERY WITH CLICKHOUSE-STYLE NAMED PARAMS ----------
  //   - we keep {paramName: Type} everywhere
  //   - we will convert to $1, $2,… in the next step
  const queryWithNamedParams = `
    SELECT
      start_time,
      usage_key,
      SUM(usage) AS usage_sum
    FROM (
      SELECT
        ${redshiftBucketExpr(bucketSizeInSeconds, "o.start_time")} AS start_time,
        f.key AS usage_key,
        CAST(f.value AS DOUBLE PRECISION) AS usage
      FROM
        mmo_schema.observations AS o
      ${tracesFilter
        ? "LEFT JOIN mmo_schema.traces AS t ON o.trace_id = t.id AND o.project_id = t.project_id"
        : ""}
      CROSS JOIN LATERAL FLATTEN(input => o.usage_details) AS f
      WHERE
        o.project_id = {projectId: String}
        ${appliedFilter.query ? `AND ${appliedFilter.query}` : ""}
        ${environmentFilter.query ? `AND ${environmentFilter.query}` : ""}
        ${
          timeFilter
            ? `AND t.timestamp >= {traceTimestamp: DateTime64(3)} - ${OBSERVATIONS_TO_TRACE_INTERVAL}`
            : ""
        }
    ) AS s
    GROUP BY
      start_time,
      usage_key
    ORDER BY start_time ASC
  `;

  // ---------- MERGE ALL NAMED PARAMS ----------
  const namedParams: Record<string, any> = {
    projectId,
    ...appliedFilter.params,
    ...environmentFilter.params,
  };

  if (timeFilter) {
    // still fine to reuse this helper; Redshift driver just sees a JS Date / string
    namedParams.traceTimestamp = convertDateToClickhouseDateTime(
      timeFilter.value,
    );
  }

  // ---------- CONVERT TO PG-STYLE ($1, $2…) + PARAM ARRAY ----------
  const { query, params } = convertNamedToPgParams(
    queryWithNamedParams,
    namedParams,
    1,
  );

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

  // ---------- POST-PROCESSING (same logic as ClickHouse version) ----------
  const allTypes = new Set<string>();
  const byBucket = new Map<string, Record<string, number>>();

  for (const row of result) {
    allTypes.add(row.usage_key);

    const bucketKey = row.start_time;
    const existing = byBucket.get(bucketKey) ?? {};
    existing[row.usage_key] =
      row.usage_sum != null ? Number(row.usage_sum) : 0;
    byBucket.set(bucketKey, existing);
  }

  const uniqueTypes = [...allTypes];
  const finalResult: {
    intervalStart: Date;
    key: string;
    sum: number;
  }[] = [];

  for (const [bucketStart, usageByType] of byBucket.entries()) {
    const intervalStart = parseClickhouseUTCDateTimeFormat(bucketStart);
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
