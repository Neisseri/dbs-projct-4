-- @Name: data
-- @Depends: table-data
-- @Flags: data
-- @Description: Load large amount of data
-- @Score: 0

USE DATASET;

LOAD DATA INFILE '/mnt/data/part.csv' INTO TABLE PART FIELDS TERMINATED BY ',';
LOAD DATA INFILE '/mnt/data/region.csv' INTO TABLE REGION FIELDS TERMINATED BY ',';
LOAD DATA INFILE '/mnt/data/nation.csv' INTO TABLE NATION FIELDS TERMINATED BY ',';
LOAD DATA INFILE '/mnt/data/supplier.csv' INTO TABLE SUPPLIER FIELDS TERMINATED BY ',';
LOAD DATA INFILE '/mnt/data/customer.csv' INTO TABLE CUSTOMER FIELDS TERMINATED BY ',';
LOAD DATA INFILE '/mnt/data/partsupp.csv' INTO TABLE PARTSUPP FIELDS TERMINATED BY ',';
LOAD DATA INFILE '/mnt/data/orders.csv' INTO TABLE ORDERS FIELDS TERMINATED BY ',';
LOAD DATA INFILE '/mnt/data/lineitem.csv' INTO TABLE LINEITEM FIELDS TERMINATED BY ',';

