-- @Name: query-aggregate
-- @Depends: optional
-- @Flags: aggregate
-- @Description: Aggregate query
-- @Score: 1
USE DATASET;

SELECT MIN(SUPPLIER.S_NATIONKEY),MAX(SUPPLIER.S_ACCTBAL),SUM(SUPPLIER.S_SUPPKEY),COUNT(*) FROM SUPPLIER WHERE SUPPLIER.S_SUPPKEY < 100;
SELECT S_NATIONKEY, COUNT(*), AVG(S_ACCTBAL) FROM SUPPLIER WHERE SUPPLIER.S_SUPPKEY < 1000 GROUP BY SUPPLIER.S_NATIONKEY;
