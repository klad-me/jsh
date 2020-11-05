function require(name)
{
	var cache = require.cache;
	if (name in cache) return cache[name];
	var exports = {};
	cache[name] = exports;
	Function('exports', read(name+'.js'))(exports);
	return exports;
}
require.cache = Object.create(null);
