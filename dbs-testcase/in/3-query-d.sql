-- @Name: query-d
-- @Depends: table
-- @Flags: query
-- @Description: Small data query with several deletions
-- @Score: 3

USE DB;

INSERT INTO TBL4 VALUES (1, '3', 1.0), (5, '5', 3.0), (1, '1', 4.0);
INSERT INTO TBL4 VALUES (2, '4', 4.0), (4, '1', 5.0), (1, '2', 2.0), (2, '4', 2.0), (5, '3', 5.0), (1, '5', 5.0);
INSERT INTO TBL4 VALUES (5, '1', 3.0), (1, '4', 3.0), (2, '5', 2.0), (3, '1', 4.0), (2, '5', 2.0), (3, '2', 5.0), (2, '3', 5.0);
INSERT INTO TBL4 VALUES (5, '1', 5.0), (1, '3', 4.0);
SELECT * FROM TBL4;
DELETE FROM TBL4 WHERE a = 1;
SELECT * FROM TBL4;
INSERT INTO TBL4 VALUES (1, '3', 1.0), (3, '3', 4.0), (5, '1', 1.0), (3, '3', 5.0), (1, '5', 4.0), (1, '3', 4.0), (1, '5', 2.0);
INSERT INTO TBL4 VALUES (1, '5', 2.0), (5, '2', 4.0), (1, '4', 4.0), (5, '4', 1.0), (4, '1', 5.0), (5, '5', 1.0);
SELECT * FROM TBL4;
DELETE FROM TBL4 WHERE a = 3 AND b = '3';
SELECT * FROM TBL4;
INSERT INTO TBL4 VALUES (1, '5', 4.0), (2, '4', 1.0), (5, '5', 2.0), (5, '1', 4.0), (4, '3', 4.0), (2, '2', 4.0);
INSERT INTO TBL4 VALUES (3, '3', 5.0), (2, '4', 5.0);
SELECT * FROM TBL4;
DELETE FROM TBL4 WHERE b = '4' AND c <= 2.0;
SELECT * FROM TBL4;
INSERT INTO TBL4 VALUES (2, '5', 2.0), (2, '2', 1.0), (1, '3', 2.0);
INSERT INTO TBL4 VALUES (2, '3', 5.0), (3, '4', 1.0), (3, '3', 4.0), (4, '4', 1.0);
SELECT * FROM TBL4;
DELETE FROM TBL4 WHERE b < '2' AND c > 3.0;
SELECT * FROM TBL4;

