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

  // you can still use this just to get the bucket size:
  const [, , bucketSizeInSeconds] = orderByTimeSeries(filter, "start_time");

  const bucketExpr = `
    DATEADD(
      second,
      FLOOR(
        DATEDIFF(
          second,
          TIMESTAMP '1970-01-01 00:00:00',
          o.start_time
        ) / ${bucketSizeInSeconds}
      ) * ${bucketSizeInSeconds},
      TIMESTAMP '1970-01-01 00:00:00'
    )
  `;

  // ---------- THIS IS THE IMPORTANT PART: UNPIVOT, NOT FLATTEN ----------
  const queryWithNamedParams = `
    SELECT
      start_time,
      usage_key,
      SUM(usage) AS usage_sum
    FROM (
      SELECT
        ${bucketExpr} AS start_time,
        usage_key,
        CAST(usage AS DOUBLE PRECISION) AS usage
      FROM
        mmo_schema.observations AS o
      ${
        tracesFilter
          ? "LEFT JOIN mmo_schema.traces AS t ON o.trace_id = t.id AND o.project_id = t.project_id"
          : ""
      }
      , UNPIVOT o.usage_details AS usage AT usage_key
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
    ORDER BY
      start_time ASC
  `;
