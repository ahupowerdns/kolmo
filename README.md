# kolmo
Libkolmo: configuration management primitives library &amp; support infrastructure

Status: Running prototpe

# Kolmogorov complexity
The name is a nod to Andrey Nikolaevich Kolmogorov who (together with
Gregory Chaitin and Ray Solomonoff) first defined [Kolmogorov complexity](https://en.wikipedia.org/wiki/Kolmogorov_complexity). In
short and highly informally, this is the amount of data needed to fully describe
the state of a system compared to a universally known background.

In mathematics, the Kolmogorov complexity of the first billion digits of Pi
is not a billion digits. It is the size of a computer program that can
calculate those billion digits.

In the world of computing, the Kolmogorov complexity of a freshly installed
Debian Stretch server is roughly the length of the following string "Install
from debian-9.1.0-amd64-netinst.iso using all defaults".

If someone logs in to the server and changes a few settings and, crucially,
writes down what changes were made, the Kolmogorov complexity of the install
increases by dozens of bytes to length of the string "Install .. using all
defaults and change sysctl x y and z".

However, the moment some random packages are installed or sysctl settings
are changed of the running kernel **without keeping track**, all bets are off, and
the Kolmogorov complexity now runs into the gigabytes: a full serialization of 
running memory and filesystem. That is, unless there is some way to retrieve the
exact running configuration.

The goal of Libkolmo is exactly that: prevent this explosion of complexity
while making software easier to configure & serialize.

## Examples
Note: these examples are not in sync with the actual code. Best source for examples right now is http://kolmo.org/ !

Assume a simple webserver called `ws` with a control command called `wsctl`.
This is based on a `ws` and `wsctl` linking in 'libkolmo', which provides 
these capabilities.

```
$ cat ws.conf
{}
$ ws
w: Listening on 127.0.0.1:80, serving from /var/www/html
$ wsctl --minimal-config
$
$ wsctl --add-vhost [::1]:80 /var/www/ipv6/
$ wsctl minimcal-config
vhost [::1]:80 {
	root /var/www/ipv6;
}

$ wsctl full-config
vhost 127.0.0.1:80 {
	root /var/www/html;
}
log-level error
log-facility daemon
daemonize
access-log off

vhost [::1]:80 {
	root /var/www/ipv6;
}

# wsctl --commit-config
$ diff $<(wsctl --dump-config) w.conf
$
```

In addition to this, `wsctl` also has a --json flag. All defaults of the `w`
server live in the Kolmo descriptors, there is no "127.0.0.1:80" anywhere in
the source. This means we can even port configuration files to new versions.
Let's say version 1.1 of wctl no longer listens on 127.0.0.1:80 by default:

```
$ wsctl --dump-config --from 1.0 --to 1.1
vhost 127.0.0.1:80 {
	root /var/www/html;
}
vhost [::1]:80 {
	root /var/www/ipv6;
}
```
Suddenly the 127.0.0.1 address is part of --dump-config, which it previously
wasn't.

## Web-based example
Query the configuration:
```
# wsctl --web-server 127.0.0.1:8080 --web-user=admin --web-password=Ux5giiSa
$ curl http://admin:Ux5giiSa@127.0.0.1:8080/vhosts | jq .
{ "vhosts": [{"address": "127.0.0.1:80", "root": "/var/www/html"},
             {"address": "[::1]:80", "root": "/var/www/ipv6"}] } 
```
Or change it:
```
$ curl -X POST http://admin:Ux5giiSa@127.0.0.1:8080/vhosts --data '{"address": "192.168.1.1:80", "root": "/var/www/rfc1918/"}'
```
And check how it ended up:
```
$ wsctl --dump-config
vhost [::1]:80 {
	root /var/www/ipv6;
}
vhost 192.168.1.1:80 {
	root /var/www/rfc1918;
}
```

## Larger problem statement
Programs have large numbers of settings, many of which with defaults. These
defaults frequently change from version to version, and they are scattered
throughout the program code.

Settings are typically stored as a configuration file, which on startup is
loaded as the 'runtime configuration'. This configuration can also often be
reloaded without a software restart. The configuration file can also be
changed while the program is running, leading to a discrepancy between
running state and stored configuration file.

Parts of the configuration file can often be changed for reload at runtime,
but some changes require a whole program restart. The documentation may show
which features these are.

A running configuration can frequently be modified at runtime
without a corresponding change in the stored configuration. Such changes,
often initiated over a CLI or an API call, are not permanent and will vanish
on a restart.

It is typically not possible to commit runtime changes to permanent
configuration.  There is typically no facility for serializing the running
configuration, and if there is, such a serialized configuration suddenly
includes all defaults.  Such a dumped running state is frequently not
portable between different versions of software.

Some programs offer the ability to 'test' a configuration file before
loading it. Such testing is often not quite the real thing, and there is no
guarantee such a tested configuration file will actually lead to a
functioning program.

It is also typically hard to programmatically change a stored configuration
state. Such operations are frequently handled via 'search and replace'
operations, or simply tacking on lines or stanzas to the end of the
configuration.

## Relation to configuration management, ansible, puppet etc
One way to limit the Kolmogorov complexity of a system is to only ever change it via a playbook.
This is, in fact, why we do playbooks: to get a description of a running system that is limited in
size, and therefore composable.

A problem of creating new playbooks is that it is a slow and laborious process. Changes get made to the playbook,
the playbook gets played, the resulting server is not quite right, and so the process continues.

With libkolmo enabled programs, the target system can be configured 'in place' until it is right, and then 
serialized with the proper commands. This serialized state is then embedded in the playbook.

As such it becomes trivial to "playbook up a running system".

### Related work

 * [Blueprint](https://github.com/devstructure/blueprint) - Reverse engineer server configuration
