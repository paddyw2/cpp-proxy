# Network Proxy
A basic network proxy with logging and packet altering options

To run:

1. <code>make</code>
2. <code>./proxy [logging options] port [replace options] host host-port</code>

Example:

<code>./proxy -hex 8080 -replace div new-div-name www.neverssl.com 80</code>
