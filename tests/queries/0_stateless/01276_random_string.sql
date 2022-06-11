-- Tags: high-memory-usage

SELECT DISTINCT c > 30000 FROM (SELECT arrayJoin(arrayMap(x -> reinterpretAsUInt8(substring(randomString(100), x + 1, 1)), range(100))) AS byte, count() AS c FROM numbers(100000) GROUP BY byte ORDER BY byte);
