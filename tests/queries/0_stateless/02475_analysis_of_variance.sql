
SELECT analysisOfVariance(number, number % 2) FROM numbers(10) FORMAT Null;
SELECT analysisOfVariance(number :: Decimal32(5), number % 2) FROM numbers(10) FORMAT Null;
SELECT analysisOfVariance(number :: Decimal256(5), number % 2) FROM numbers(10) FORMAT Null;

SELECT analysisOfVariance(1.11, -20); -- { serverError BAD_ARGUMENTS }
SELECT analysisOfVariance(1.11, 20 :: UInt128); -- { serverError BAD_ARGUMENTS }


