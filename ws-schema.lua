
main:registerVariable("verbose", "bool", { default="true", runtime="true", cmdline="-v", description="Perform verbose logging"})

main:registerVariable("server-name", "string", {default="", runtime="true", description="Name this server reports as by default"})

main:registerVariable("client-timeout", "integer",
                                        {
                                                default="5000",
                                                runtime="true",
                                                unit="milliseconds",
                                                description="Timeout before client gets disconnected",
                                                check='if(x < 1) then error("Timeout must be at least one millisecond") end'
                                        })
main:registerVariable("max-connections", "integer", {default="200", runtime="false", unit="connections", description="Maximum number of versions"})
main:registerVariable("hide-server-version", "bool", {default="false", runtime="true", description="If we should hide server version number"})
main:registerVariable("hide-server-type", "bool", {default="false", runtime="true", description="If we should hide server type"})

main:registerVariable("carbon-server", "ipendpoint", {default="", runtime="true", description="Send performance metrics to this IP address"})


site=createClass("site", {})
site:registerVariable("name", "string", { runtime="false", description="Hostname of this website"})
site:registerVariable("enabled", "bool", { runtime="false", default="true", description="If this site is enabled"})
site:registerVariable("path", "string", { runtime="true", description="Path on filesystem where content is hosted"})
site:registerVariable("listen", "struct", { member_type="ipendpoint", runtime="false", description="IP endpoints we listen on"})
site:registerVariable("redirect-to-https", "bool", { default="false", runtime="true", description="If all http requests should be redirected to https"})

sites=main:registerVariable("sites", "struct", {member_type="site", runtime="false", description="Sites we serve"})

listener=createClass("listener", {})
listener:registerVariable("tls", "bool", { runtime="false", description="If this listener should perform TLS"})
listener:registerVariable("fast-open", "bool", { runtime="false", description="If this listener should support TCP fast open"})
listener:registerVariable("accept-filter", "bool", { runtime="false", description="If this listener should install a data-ready accept filter"})
listener:registerVariable("cert-file", "string", { runtime="false", description="Filename of certificate"})
listener:registerVariable("key-file", "string", { runtime="false", description="Filename of key"})
listener:registerVariable("pem-file", "string", { runtime="false", description="PEM file"})

listeners=main:registerVariable("listeners", "struct", { runtime="false", member_type="listener", description="Optional configurations per IP address listener"})


logger=createClass("logger", {})
logger:registerVariable("syslog", "bool", { default="true", runtime="false", description="If this logger should emit to syslog"})
logger:registerVariable("syslog-facily", "string", { default="daemon", runtime="false", description="If this logger should emit to syslog"})
logger:registerVariable("log-errors", "bool", { default="true", runtime="true", description="If this logger should log errors"})
logger:registerVariable("log-warning", "bool", { default="true", runtime="true", description="If this logger should log warnings"})
logger:registerVariable("log-hits", "bool", { default="false", runtime="true", description="If this logger should log website hits"})
logger:registerVariable("log-file", "string", { default="", runtime="false", description="Logger logs to this file"})

main:registerVariable("loggers", "struct", { runtime="false", member_type="logger", description="Loggers that log events and hits" } )
loggers=main:getStruct("loggers")
logger=loggers:getNewMember()
loggers:registerStructMember("messages", logger)

main:registerVariable("kolmo-server", "ipendpoint", {default="127.0.0.1:1234", runtime="false", description="If we should launch a kolmo server"})