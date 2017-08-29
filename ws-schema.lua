
main:registerBool("verbose", true)
main:registerString("server-name", "kolmo")

site=createClass("site", {
			name={"string", ""},
			enabled={"bool", "true"},
			path={"string", ""},
			listen={"string", ""}
			})

empty=createClass("empty", {})


sites=main:registerStruct("sites", "empty")



