-- @Name: fk
-- @Depends: pk
-- @Flags: fk
-- @Description: Basic foreign key testing
-- @Score: 2

USE DB2;


-- Check single fk
CREATE TABLE T3 (ID INT, T1_ID INT, NAME VARCHAR(16), PRIMARY KEY (ID), FOREIGN KEY (T1_ID) REFERENCES T1(ID));
CREATE TABLE T4 (ID INT, T3_ID INT, NAME VARCHAR(16), FOREIGN KEY (T3_ID) REFERENCES T3(ID));

-- Check basic insertion (and prepare data for next checks)
INSERT INTO T1 VALUES (100, '100'), (101, '101'), (102, '102');
INSERT INTO T3 VALUES (10, 100, '10_100'), (11, 101, '11_101'), (12, 102, '12_102');
INSERT INTO T4 VALUES (0, 10, '0_10_100'), (1, 11, '1_11_101');

-- Check table update or insert integrity
UPDATE T4 SET T3_ID = 12, NAME = '1_12_102' WHERE T4.ID = 1;
INSERT INTO T4 VALUES (2, 12, '2_12_102');
UPDATE T4 SET T3_ID = 13, NAME = '1_13_10?' WHERE T4.ID = 1;
INSERT INTO T4 VALUES (3, 13, '2_13_10?');
SELECT * FROM T4;
-- Line 1, 2 succeeded but line 3, 4 failed

