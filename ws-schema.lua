
main:registerVariable("verbose", "bool", { default="true", runtime="true", cmdline="-v", description="Perform verbose logging"})

main:registerVariable("server-name", "string", {default="", runtime="true", description="Name this server reports as by default"})

main:registerVariable("client-timeout", "integer", {default="5000", runtime="false", unit="milliseconds", description="Timeout before client gets disconnected"})

main:registerVariable("carbon-server", "ipendpoint", {default="", runtime="true", description="Send performance metrics to this IP address"})


empty=createClass("empty", {})


--[[
site=createClass("site", {
			name={"string", ""},
			enabled={"bool", "true"},
			path={"string", ""},
			listen={"struct", "empty"}
			})

]]--

site=createClass("site", {})
site:registerVariable("name", "string", { runtime="false", description="Hostname of this website"})
site:registerVariable("enabled", "bool", { runtime="false", default="true", description="If this site is enabled"})
site:registerVariable("path", "string", { runtime="true", description="Path on filesystem where content is hosted"})
site:registerVariable("listen", "struct", { runtime="false", description="IP endpoints we listen on"})
site:registerVariable("redirect-to-https", "bool", { default="false", runtime="true", description="If all http requests should be redirected to https"})

sites=main:registerStruct("sites", "empty")

listener=createClass("listener", {})
listener:registerVariable("tls", "bool", { runtime="false", description="If this listener should perform TLS"})
listener:registerVariable("cert-file", "string", { runtime="false", description="Filename of certificate"})
listener:registerVariable("key-file", "string", { runtime="false", description="Filename of key"})
listener:registerVariable("pem-file", "string", { runtime="false", description="PEM file"})

listeners=main:registerStruct("listeners", "empty")

