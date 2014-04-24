package org.jenkinsci.plugins.ios.connector.DeployBuilder

def f = namespace(lib.FormTagLib)

f.entry(title:"Path to .ipa file(s)", field:"path") {
    f.textbox()
}

f.entry(title:"Node(s) used:", field:"nodes") {
    f.textbox()
}