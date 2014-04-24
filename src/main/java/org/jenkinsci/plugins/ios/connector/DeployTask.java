package org.jenkinsci.plugins.ios.connector;

import hudson.FilePath;
import hudson.Launcher.LocalLauncher;
import hudson.Launcher.ProcStarter;
import hudson.Util;
import hudson.model.TaskListener;
import hudson.remoting.Callable;
import hudson.util.ArgumentListBuilder;

import java.io.File;
import java.io.IOException;
import java.io.InterruptedIOException;
import java.util.List;

/**
 * Deploys *.ipa to the device.
 *
 * @author Kohsuke Kawaguchi
 */
class DeployTask implements Callable<Void, IOException> {

    private final FilePath bundle;
    private final TaskListener listener;
    private final String deviceId;
    private final FilePath rootPath;

    DeployTask(iOSDevice device, File bundle, TaskListener listener) {
        this.bundle = new FilePath(bundle);
        this.listener = listener;
        this.deviceId = device.getUniqueDeviceId();
        this.rootPath = device.getComputer().getNode().getRootPath();
    }

    public Void call() throws IOException {
        File t = Util.createTempDir();
        try {
            FilePath fruitstrap = rootPath.child("fruitstrap");
            if (!fruitstrap.exists() || !fruitstrap.digest().equals(FRUITSTRAP_DIGEST)) {
                listener.getLogger().println("Extracting fruitstrap to "+fruitstrap);
                fruitstrap.copyFrom(DeployTask.class.getResource("fruitstrap"));
                fruitstrap.chmod(0755);
            }

            listener.getLogger().println("Copying "+ bundle +" to "+ t);

            // Determine what type of file was passed
            final String filename = bundle.getName();

            ArgumentListBuilder arguments = new ArgumentListBuilder(fruitstrap.getRemote());
            arguments.add("--id", deviceId, "--bundle", filename);

            ProcStarter proc = new LocalLauncher(listener).launch()
                    .cmds(arguments)
                    .stdout(listener)
                    .pwd(bundle.getParent());
            int exit = proc.join();
            if (exit!=0)
                throw new IOException("Deployment of "+bundle+" failed: "+exit);

            return null;
        } catch (InterruptedException e) {
            throw (IOException)new InterruptedIOException().initCause(e);
        } finally {
            Util.deleteRecursive(t);
            listener.getLogger().flush();
        }
    }

    private static final String FRUITSTRAP_DIGEST;

    static {
        try {
            FRUITSTRAP_DIGEST = Util.getDigestOf(DeployTask.class.getResourceAsStream("fruitstrap"));
        } catch (IOException e) {
            throw new Error("Failed to compute the digest of fruitstrap",e);
        }
    }
}
