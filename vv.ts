import z from "zod/v4";

/**
 * Helper: format Date -> "YYYY-MM-DD HH:mm:ss.SSS" in UTC (Redshift-friendly)
 */
function formatDateForRedshift(d: Date): string {
  // ensure UTC values
  const year = d.getUTCFullYear();
  const month = String(d.getUTCMonth() + 1).padStart(2, "0");
  const day = String(d.getUTCDate()).padStart(2, "0");
  const hours = String(d.getUTCHours()).padStart(2, "0");
  const minutes = String(d.getUTCMinutes()).padStart(2, "0");
  const seconds = String(d.getUTCSeconds()).padStart(2, "0");
  const ms = String(d.getUTCMilliseconds()).padStart(3, "0");
  return `${year}-${month}-${day} ${hours}:${minutes}:${seconds}.${ms}`;
}

/**
 * Redshift timestamp schema: accepts number|string|Date -> outputs formatted string
 *
 * - If number: treated as epoch ms
 * - If string: parsed with Date constructor (ISO / any parseable string)
 * - If Date: used directly
 */
export const redshiftTimestampSchema = z.preprocess((arg) => {
  if (typeof arg === "number") return new Date(arg);
  if (typeof arg === "string") return new Date(arg);
  if (arg instanceof Date) return arg;
  return arg;
}, z.date()).transform((d) => formatDateForRedshift(d));

/* ---------------------------------------------------------------------
   Create a Redshift insert schema for TraceRecords (same base + transformed ts)
   --------------------------------------------------------------------- */

export const traceRecordRedshiftInsertSchema = traceRecordBaseSchema.extend({
  timestamp: redshiftTimestampSchema,
  created_at: redshiftTimestampSchema,
  updated_at: redshiftTimestampSchema,
  event_ts: redshiftTimestampSchema,
});

export type TraceRecordRedshiftInsertType = z.infer<
  typeof traceRecordRedshiftInsertSchema
>;
