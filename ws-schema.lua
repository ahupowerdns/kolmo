
main:registerBool("verbose", true)
main:registerString("server-name", "kolmo")

local site=createClass("site", {
			name={"string", ""},
			enabled={"bool", "true"},
			path={"string", ""},
			listen={"string", ""}
			})


local sites=main:registerVector("sites", "site")

local site = sites:create()

site:setString("name", "kolmo.org")
site:setString("path", "/var/www/kolmo.org")
site:setString("listen", "[::]:8000")

sites:append(site)

site:setString("name", "ds9a.nl")
site:setString("path", "/var/www/ds9a.nl")
site:setString("listen", "[::]:8001")

sites:append(site)





