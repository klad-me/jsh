#! jsh

// fails
var result=exec( /*["ls", "-la", "/"],*/ ["grep", "4096"], ["grep", "-v", "mnt"], "hello");

echo(JSON.stringify(result, null, "  "));
