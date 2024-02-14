-- @Name: comb-pk
-- @Depends: query-a
-- @Flags: comb pk
-- @Description: Combined primary key testing
-- @Score: 2

USE DB2;


-- Check create table with compound pk
CREATE TABLE T2 (ID1 INT, ID2 INT, NAME VARCHAR(16), PRIMARY KEY (ID1, ID2));

-- Check duplication handle
INSERT INTO T2 VALUES (1, 1, '1'), (1, 2, '2'), (1, 3, '3');
INSERT INTO T2 VALUES (2, 1, '1'), (2, 2, '2'), (2, 3, '3');
INSERT INTO T2 VALUES (1, 1, '7');
SELECT * FROM T2;
INSERT INTO T2 VALUES (3, 1, '7');
SELECT * FROM T2;
