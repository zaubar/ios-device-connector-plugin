ios-device-connector-plugin
===========================

This is a fork of the Jenkins iOS Device Connetor Plugin with modified functionality.

https://wiki.jenkins-ci.org/display/JENKINS/iOS+Device+Connector+Plugin

Overview
--------------

**NOTE**: This fork has been modified to only work with ipas (every device listed in the 
connected ios devices list on the Jenkins front page).

1. The specified ipa(s) will be deployed to all connected devices. Wildcards(*) are still supported.

2. **Optional** Restrict which nodes with connected devices will be used for deployment.
This is specified as a space-delimited list of node names (labels not supported). The
plugin will verify each name before starting the deployment process. 

If no list (or no valid node) is provided then the default will be to deploy to all devices.

3. **Optional** Specify a list of bundle identifiers to uninstall apps prior to deployment. This
allows a more controlled environment emulating a clean install.