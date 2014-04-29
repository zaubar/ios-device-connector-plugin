ios-device-connector-plugin
===========================

This is a fork of the Jenkins iOS Device Connetor Plugin with modified functionality.

https://wiki.jenkins-ci.org/display/JENKINS/iOS+Device+Connector+Plugin

The plugin has been modified to provide the following functionality:

0. As a background note, this fork has been modified to only work with ipas. There were some issues
with the unzip and deploy process so the middle-man has been cut out and fruitstrap wil
deploy the ipa directly. This means that .app files cannot be deployed.

1. The specified ipa(s) will be deployed to all connected devices according to
restrictions specified in step (2)

2. **Optional** Restrict which nodes that have connected devices, will be used for deployment.
You can specify a space-delimited list of node names (labels not supported), and the
plugin will verify before starting the deployment process. If no list (or no valid node)
is provided then the default will be to deploy to all connected to devices.

3. **Optional** Specify a list of bundle identifiers to uninstall apps prior to deployment. This
allows a more controlled environment emulating a clean install.