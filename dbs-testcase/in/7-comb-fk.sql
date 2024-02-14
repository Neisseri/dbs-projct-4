-- @Name: comb-fk
-- @Depends: comb-pk
-- @Flags: comb fk
-- @Description: Combined foreign key testing
-- @Score: 2

USE DB2;

CREATE TABLE T5 (ID INT, T2_ID1 INT, T2_ID2 INT, NAME VARCHAR(16), FOREIGN KEY (T2_ID1, T2_ID2) REFERENCES T2(ID1, ID2));

-- Check basic insertion (and prepare data for next checks)
INSERT INTO T2 VALUES (10, 10, '10_10'), (10, 11, '10_11'), (11, 10, '11_10'), (11, 11, '11_11');
INSERT INTO T5 VALUES (1, 10, 10, '1_10_10'), (2, 10, 10, '2_10_10'), (3, 10, 11, '3_10_11'), (4, 11, 10, '4_11_10');


-- Check reference table update or delete integrity
UPDATE T2 SET ID2 = 12 WHERE T2.ID1 = 10 AND T2.ID2 = 10;

UPDATE T2 SET ID2 = 12 WHERE T2.ID1 = 11 AND T2.ID2 = 11;

DELETE FROM T2 WHERE T2.ID2 = 11;

DELETE FROM T2 WHERE T2.ID2 = 12;
-- Line 1, 3 failed but line 2, 4 succeeded 
