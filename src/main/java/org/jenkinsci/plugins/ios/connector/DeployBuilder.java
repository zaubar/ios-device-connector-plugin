package org.jenkinsci.plugins.ios.connector;

import hudson.Extension;
import hudson.FilePath;
import hudson.Launcher;
import hudson.Util;
import hudson.model.AbstractBuild;
import hudson.model.AbstractProject;
import hudson.model.BuildListener;
import hudson.model.Computer;
import hudson.tasks.BuildStepDescriptor;
import hudson.tasks.Builder;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import javax.inject.Inject;
import jenkins.model.Jenkins;
import org.kohsuke.stapler.DataBoundConstructor;

public class DeployBuilder extends Builder {
    public final String bundleId;
    public final String path;
    public final String nodes;

    @Inject
    private transient iOSDeviceList devices;

    @DataBoundConstructor
    public DeployBuilder(String path,String bundleId,String nodes) {
        this.path = path;
        this.bundleId = bundleId;
        this.nodes = nodes;
    }

    @Override
    public boolean perform(AbstractBuild<?, ?> build, Launcher launcher, BuildListener listener) throws InterruptedException, IOException {
        Jenkins.getInstance().getInjector().injectMembers(this);

        // Expand matrix and build variables in the device ID and command line args
        final String expandPath = expandVariables(build, listener, path);
        final String expandBundle = expandVariables(build, listener, bundleId);
        final String expandNodes = expandVariables(build, listener, nodes);
        List<String> nodeList = getRestrictedNodes(expandNodes, listener);
        
        // Setup the workspace path and collect app files, fail quickly if there are any problems.
        final FilePath ws = build.getWorkspace();
        if(ws == null) {
            throw new NullPointerException("Workspace path is null.");
        }
        
        FilePath[] files = getAppFiles(ws, expandPath);
        if (files.length == 0) {
            listener.getLogger().println("No iOS apps found to deploy!");
            return false;
        }
        
        listener.getLogger().printf("DEPLOY DEVICE COUNT: %s%n", devices.getDevices().size());
        for(Map.Entry<Computer, iOSDevice> dev : devices.getDevices().entries()) {
            iOSDevice device = dev.getValue();
            String node = dev.getKey().getName();
            String display = dev.getKey().getDisplayName();
            if(nodeList.isEmpty() || nodeList.contains(node)) {
                listener.getLogger().printf("Node Name: %s%n", node);
                listener.getLogger().printf("Node Display: %s%n", display);
                listener.getLogger().printf("Path: %s%n", expandPath);
                listener.getLogger().printf("Device: %s%n", device.getDisplayName());

				try {
                    deployApps(files, expandBundle, device, listener);
				} catch (IOException e) {
                    listener.getLogger().printf("Error while deploying %s%n", e.toString());
				}
            } else {
                listener.getLogger().printf("Skipping deployment to: %s%n", node);
            }
        }
        return true;
    }
    
    private static List<String> getRestrictedNodes(String nodeList, BuildListener listener) {
        List<String> result = new ArrayList<String>();
        
        if(nodeList != null && !nodeList.isEmpty()) {
            listener.getLogger().printf("=NODE RESTRICTION=%n");
            for(String part : nodeList.split(" ")) {
                if(isRealNode(part)) {
                    result.add(part);
                    listener.getLogger().printf("\t- %s%n", part);
                } else {
                    listener.getLogger().printf("Invalid Node Name. %s%n", part);
                }
            }
        } else {
            listener.getLogger().printf("No specified nodes. Deploying to all.%n");
        }
        return result;
    }
    
    private static boolean isRealNode(String name) {
        Computer node = hudson.model.Hudson.getInstance().getComputer(name);
        
        return node != null;
    }
    
    private static FilePath[] getAppFiles(FilePath ws, String expandPath) throws InterruptedException, IOException {
        return ws.child(expandPath).exists() ? new FilePath[]{ws.child(expandPath)} : ws.list(expandPath);
    }
    
    private static void deployApps(FilePath[] files, String bundleId, iOSDevice device, BuildListener listener) throws IOException, InterruptedException {
        for (FilePath bundle : files) {
            String name = bundle.getName();
            int idx = name.lastIndexOf('.');
            if (idx < 0) {
                listener.getLogger().printf("Ignoring '%s'; expected either a .app or .ipa bundle%n", name);
                continue;
            }

            
            listener.getLogger().printf("Deploying iOS app: %s%n", name);
            device.deploy(new File(bundle.getRemote()), bundleId, listener);
        }
    }

    private static String expandVariables(AbstractBuild<?, ?> build, BuildListener listener, String token) {
        Map<String, String> vars = build.getBuildVariables();
        try {
            vars.putAll(build.getEnvironment(listener));
        } catch (IOException e) {
            return null;
        } catch (InterruptedException e) {
            return null;
        }

        String result = Util.fixEmptyAndTrim(token);
        if (result != null) {
            result = Util.replaceMacro(result, vars);
        }
        return result;
    }

    @Extension
    public static class DescriptorImpl extends BuildStepDescriptor<Builder> {
        @Override
        public String getDisplayName() {
            return "Deploy iOS App to all connected devices";
        }

        @Override
        public boolean isApplicable(Class<? extends AbstractProject> jobType) {
            return true;
        }
    }
}
