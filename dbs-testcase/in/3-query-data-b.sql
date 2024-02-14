-- @Name: query-data-b
-- @Depends: data
-- @Flags: query
-- @Description: large table read and write
-- @Score: 3

USE DATASET;

INSERT INTO PART VALUES (60001, 'anything', 'Manufacturer#5', 'Brand#28', 'PROMO PLATED COPPER', 34, 'JUST WORD', 1364.23, 'SOMETHING'), (60002, 'anything', 'Manufacturer#1', 'Brand#12', 'PROMO PLATED COPPER', 34, 'JUST WORD', 1364.23, 'SOMETHING'), (60003, 'anything', 'Manufacturer#7', 'Brand#39', 'PROMO PLATED COPPER', 34, 'JUST WORD', 1525.23, 'SOMETHING'), (60004, 'anything', 'Manufacturer#5', 'Brand#21', 'PROMO PLATED COPPER', 34, 'JUST WORD', 1743.13, 'SOMETHING'),(60005, 'anything', 'Manufacturer#5', 'Brand#30', 'PROMO PLATED COPPER', 34, 'JUST WORD', 1636.35, 'SOMETHING');

SELECT * FROM PART WHERE P_BRAND = 'Brand#30';
SELECT P_PARTKEY, P_BRAND, P_RETAILPRICE FROM PART WHERE P_BRAND >= 'Brand#271' AND P_BRAND <= 'Brand#300';
SELECT P_SIZE, P_PARTKEY, P_NAME FROM PART WHERE P_BRAND = 'Brand#13' AND P_SIZE > 49;
SELECT * FROM PART WHERE P_RETAILPRICE > 1525.1 AND P_RETAILPRICE < 1525.9;
DELETE FROM PART WHERE P_BRAND = 'Brand#30';
SELECT P_BRAND FROM PART WHERE P_BRAND > 'Brand#27' AND P_BRAND < 'Brand#31';
