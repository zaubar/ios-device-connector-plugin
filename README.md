ios-device-connector-plugin
===========================

This is a fork of the Jenkins iOS Device Connetor Plugin with modified functionality.

https://wiki.jenkins-ci.org/display/JENKINS/iOS+Device+Connector+Plugin

The plugin has been modified to provide the following functionality:

1. The specified ipa(s) will be deployed to all connected devices according to
restrictions specified in step (2)

2. Restrict which nodes that have connected devices, will be used for deployment.
You can specify a space-delimited list of node names (labels not supported), and the
plugin will verify before starting the deployment process. If no list (or no valid node)
is provided then the default will be to deploy to all connected to devices.

3. As a background note, this fork skips the unzip and only works with ipas
