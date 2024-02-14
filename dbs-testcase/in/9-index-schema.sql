-- @Name: index-schema
-- @Depends: query-data-a
-- @Flags: index
-- @Description: Schema alter for indexes
-- @Score: 4

USE DB;

INSERT INTO TBL9 VALUES (1, '1', 1.0);
INSERT INTO TBL9 VALUES (2, '2', 2.0), (3, '3', 3.0), (4, '4', 4.0), (5, '5', 5.0), (6, '6', 6.0);
INSERT INTO TBL9 VALUES (2, '2', 2.0), (3, '3', 3.0);

-- Check add index and show desc
ALTER TABLE TBL9 ADD INDEX idx_a(a);
DESC TBL9;

SELECT * FROM TBL9;
SELECT * FROM TBL9 WHERE a > 3;
SELECT * FROM TBL9 WHERE a = 3;
SELECT * FROM TBL9 WHERE a < 3 AND a > 4;
SELECT * FROM TBL9 WHERE a > 3 AND a < 4;

-- Check index with duplicated data
INSERT INTO TBL9 VALUES (2, '2', 2.0), (3, '3', 3.0), (4, '4', 4.0), (5, '5', 5.0), (6, '6', 6.0);
SELECT * FROM TBL9 WHERE a > 2 AND a < 6;


-- Check drop index
ALTER TABLE TBL9 DROP INDEX idx_a;
DESC TBL9;
SELECT * FROM TBL9 WHERE a > 2 AND a < 6;

