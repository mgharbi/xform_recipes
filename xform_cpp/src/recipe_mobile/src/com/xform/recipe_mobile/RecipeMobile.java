package com.xform.recipe_mobile;

import android.app.Activity;
import android.app.ProgressDialog;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.os.Bundle;
import android.os.SystemClock;
import android.util.Log;
import android.view.View;
import android.widget.ArrayAdapter;
import android.widget.ImageView;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.EditText;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.File;
import java.io.InputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.lang.Thread;
import java.math.BigInteger;
import java.net.MalformedURLException;
import java.net.HttpURLConnection;
import java.net.URL;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import com.xform.recipe_mobile.R;
import com.xform.recipe_mobile.InputPreprocessing;
import com.xform.recipe_mobile.Precomputation;

public class RecipeMobile extends Activity
{

    private final String server_root               = "http://82.216.110.103/";

    private final String naiveCloudProcessing_url  = server_root+"naive_cloud";
    private final String recipeCloudProcessing_url = server_root+"recipe_cloud";
    private final String pingURL                   = server_root+"ping";

    // Image data
    private Bitmap unprocessed_image;
    private Bitmap processed_image;
    private Bitmap aux_image;

    // Recipe Parameters
    private Bitmap recipe_hp;
    private Bitmap recipe_lp;
    private float[] qtable;
    private int scaleFactor = 4;
    private int inputQuality = 60;

    private long JNIRecipePtr = 0; 

    // UI
    ImageView image_preview;
    private Spinner sAlgorithms;
    private TextView status_indicator;

    // Filter parameters
    int local_laplacian_levels    = 40;
    float local_laplacian_alpha   = 10.0f;
    int style_transfer_levels     = 15;
    int style_transfer_iterations = 3;

    /** Get the name of the currently selected processing algorithm. */
    String currentAlgo() {
        return sAlgorithms.getSelectedItem().toString();
    }

    /** Called when the activity is first created. */
    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);

        // Image feedback view
        image_preview    = (ImageView)findViewById(R.id.image_preview);
        status_indicator = (TextView)findViewById(R.id.status_indicator);

        // Populate list of algorithms
        List<String> algorithm_list = new ArrayList<String>();
        algorithm_list.add("local_laplacian");
        algorithm_list.add("style_transfer");
        algorithm_list.add("colorization");
        algorithm_list.add("time_of_day");
        algorithm_list.add("portrait_transfer");

        ArrayAdapter<String> adapter = new ArrayAdapter<String>(
            this, android.R.layout.simple_spinner_item, algorithm_list);
        adapter.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item);
        sAlgorithms = (Spinner) findViewById(R.id.algorithms_spinner);
        sAlgorithms.setAdapter(adapter);
        sAlgorithms.setSelection(0);
    }


    /** Update the bitmap being displayed */
    public void updatePreview(Bitmap b) {
        image_preview.setImageBitmap(b);
    }

    /** Download a new input image from the remote server */
    public void downloadFile(View v){
        status_indicator.setText("Downloading input...");
        try {
            long t_start = SystemClock.uptimeMillis();

            // Path to the remote input file
            String remote_unprocessed_file   = server_root+"/data/"+currentAlgo()+".jpg";

            // Download input image
            URL url                         = new URL(remote_unprocessed_file);
            HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
            BufferedInputStream is          = new BufferedInputStream(urlConnection.getInputStream());
            unprocessed_image               = BitmapFactory.decodeStream(is);
            is.close();

            updatePreview(unprocessed_image);
            int content_length = urlConnection.getContentLength();

            // Download additional data if required by the current algorithm
            if(currentAlgo().equals("style_transfer")) {
                String remote_target_file = server_root+"/data/style_target.jpg";
                url                       = new URL(remote_target_file);
                urlConnection             = (HttpURLConnection) url.openConnection();
                is                        = new BufferedInputStream(urlConnection.getInputStream());
                aux_image                 = BitmapFactory.decodeStream(is);
                content_length = content_length + urlConnection.getContentLength();
                is.close();
            } else if(currentAlgo().equals("colorization")) {
                String remote_target_file = server_root+"/data/scribbles.jpg";
                url                       = new URL(remote_target_file);
                urlConnection             = (HttpURLConnection) url.openConnection();
                is                        = new BufferedInputStream(urlConnection.getInputStream());
                aux_image                 = BitmapFactory.decodeStream(is);
                content_length = content_length + urlConnection.getContentLength();
                is.close();
            }

            long t_stop       = SystemClock.uptimeMillis();

            // Allocate bitmap for the output image
            processed_image = Bitmap.createBitmap(unprocessed_image.getWidth(),
                unprocessed_image.getHeight(),
                unprocessed_image.getConfig());

            status_indicator.setText(content_length/1024+"kB downloaded in "+(t_stop-t_start)+"ms");
        } catch (Exception e1) {
            e1.printStackTrace();
        }
    }


    /** Load the native cpp code library. */
    static {
        System.loadLibrary("RecipeMobileJNI");
    }
    private native void filterLocalLaplacian(Bitmap input, Bitmap output, int levels, float alpha);
    private native void filterStyleTransfer(Bitmap bitmap, Bitmap target, Bitmap output, int levels, int iterations);
    private native void filterColorization(Bitmap bitmap, Bitmap scribbles, Bitmap output);
    private native void recipeReconstruct(Bitmap recipeCoefs, Bitmap lowpassResidual, float[] qTable, Bitmap output, long recipePtr);
    private native void destroyRecipeState(long recipePtr);

    /** Process input image locally on the device */
    public void localProcessing(View view){
        if (unprocessed_image == null) {
        status_indicator.setText("Cannott apply filter: no input loaded.");
            return;
        }

        // Select filter and process
        long t_start_global = SystemClock.uptimeMillis();
        if (currentAlgo().equals("local_laplacian")) {
            Log.i("RecipeMobile","Local Laplacian filtering...");
            filterLocalLaplacian(unprocessed_image, processed_image, local_laplacian_levels, local_laplacian_alpha);
        } else if (currentAlgo().equals("style_transfer")) {
            if (aux_image == null) {
            status_indicator.setText("Can't transfer style: no target loaded.");
                return;
            }
            Log.i("RecipeMobile","Style Transfer filtering...");
            filterStyleTransfer(unprocessed_image, aux_image, processed_image, style_transfer_levels, style_transfer_iterations);
        } else if (currentAlgo().equals("colorization")) {
            if (aux_image == null) {
            status_indicator.setText("Can't colorize style: no scribbles loaded.");
                return;
            }
            Log.i("RecipeMobile","Colorization filtering...");
            filterColorization(unprocessed_image, aux_image, processed_image);
        } else {
            status_indicator.setText("No local implementation.");
            return;
        }
        long t_stop = SystemClock.uptimeMillis();
        status_indicator.setText("Local processing in "+(t_stop-t_start_global)+"ms");

        updatePreview(processed_image);
    }


    /** Write image info and algorithm parameters to a Http request header */
    public void writeRequestHeader(OutputStream out){
        try {
            byte[] dimensions;
            int width = unprocessed_image.getWidth();
            int height = unprocessed_image.getHeight();
            dimensions = ByteBuffer.allocate(4*3).putInt(width).putInt(height).putInt(scaleFactor).array();
            out.write(dimensions);

            // Write filter type and parameters
            byte[] algoFlag = {-1};
            byte[] algoParams = {-1};
            if (currentAlgo().equals("local_laplacian")) {
                algoFlag = ByteBuffer.allocate(4).putInt(0).array();
                algoParams = ByteBuffer.allocate(8).putInt(local_laplacian_levels).putFloat(local_laplacian_alpha).array();
                out.write(algoFlag);
                out.write(algoParams);
            } else if (currentAlgo().equals("style_transfer")) {
                algoFlag = ByteBuffer.allocate(4).putInt(1).array();
                algoParams = ByteBuffer.allocate(8).putInt(style_transfer_levels).putInt(style_transfer_iterations).array();
                out.write(algoFlag);
                out.write(algoParams);
            } else if (currentAlgo().equals("colorization")) {
                algoFlag = ByteBuffer.allocate(4).putInt(2).array();
                out.write(algoFlag);
            } else if (currentAlgo().equals("time_of_day")) {
                algoFlag = ByteBuffer.allocate(4).putInt(3).array();
                out.write(algoFlag);
            } else if (currentAlgo().equals("portrait_transfer")) {
                algoFlag = ByteBuffer.allocate(4).putInt(4).array();
                out.write(algoFlag);
            }
        } catch(IOException e) {
            e.printStackTrace();
        }
    }


    /** Ping server to debug network speed */
    public void ping(int payload) {
        SystemClock.sleep(1000);
        try {
            URL url = new URL(pingURL);
            HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
            try {
                long t_start = SystemClock.uptimeMillis();
                urlConnection.setDoOutput(true);
                urlConnection.setRequestProperty("Accept-Encoding", "identity");

                payload = payload*1024;
                urlConnection.setFixedLengthStreamingMode(payload);

                OutputStream out = new BufferedOutputStream(urlConnection.getOutputStream());
                byte[] data = new byte[payload];
                out.write(data);
                out.close();

                /* Get response */
                BufferedInputStream in = new BufferedInputStream(urlConnection.getInputStream());
                in.close();

                long elapsed = SystemClock.uptimeMillis() - t_start;

                status_indicator.setText("Ping time: "+(elapsed)+"ms for "+payload/1024+"kB");
            } catch(Exception e) {
                e.printStackTrace();
            } finally {
                urlConnection.disconnect();
            }

        } catch(Exception e) {
            e.printStackTrace();
        }
    }

    /** UI hook for ping action */
    public void pingServer(View v) {
        int payload = Integer.parseInt(((EditText)findViewById(R.id.pingSize)).getText().toString());
        ping(payload);
    }


    /** Process image on the cloud using JPEG for both upload and download */
    public void naiveCloudProcessing(View view){
        if (unprocessed_image == null) {
            status_indicator.setText("Can't filter: no input loaded.");
            return;
        }
        try {
            long t_start_global = SystemClock.uptimeMillis();
            URL url = new URL(naiveCloudProcessing_url);
            HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
            try {
                long t_start = SystemClock.uptimeMillis();
                Log.i("RecipeMobile", "Uploading input to server");
                urlConnection.setDoOutput(true);

                long t_start_up = SystemClock.uptimeMillis();
                OutputStream out = new BufferedOutputStream(urlConnection.getOutputStream());

                writeRequestHeader(out);

                // Write input image
                unprocessed_image.compress(Bitmap.CompressFormat.JPEG, 90, out);
                out.close();
                Log.i("RecipeMobile", "Upload time: "+(SystemClock.uptimeMillis()-t_start_up)+"ms");

                // Get response
                long t_start_down = SystemClock.uptimeMillis();
                BufferedInputStream in = new BufferedInputStream(urlConnection.getInputStream());

                // Parse header
                Map<String,List<String>> header = urlConnection.getHeaderFields();

                // Unpack image
                processed_image = BitmapFactory.decodeStream(in);
                Log.i("RecipeMobile", "Download time: "+(SystemClock.uptimeMillis()-t_start_down)+"ms");

                in.close();
                long t_stop = SystemClock.uptimeMillis();

                updatePreview(processed_image);
                status_indicator.setText("Remote(naive) processing in "+(t_stop-t_start_global)+"ms");
            } catch(Exception e) {
                e.printStackTrace();
            } finally {
                urlConnection.disconnect();
            }

        } catch(Exception e) {
            e.printStackTrace();
        }
    }

    /** Our method, Process image on the cloud using degraded JPEG for upload and recipe for download */
    public void recipeCloudProcessing(View view){
        if (unprocessed_image == null) {
            status_indicator.setText("Cannot apply filter: no input loaded.");
            return;
        }
        try {
            long t_start_global = SystemClock.uptimeMillis();
            URL url = new URL(recipeCloudProcessing_url);
            Log.i("RecipeMobile", "");
            Log.i("RecipeMobile", "--- Recipe processing ---");
            HttpURLConnection urlConnection = (HttpURLConnection) url.openConnection();
            try {

                // Compute histograms of the full quality input while we upload the (degraded) proxy input
                InputPreprocessing rPre    = new InputPreprocessing(unprocessed_image, scaleFactor);
                Thread preprocessingThread = new Thread(rPre, "preprocessing thread");
                preprocessingThread.start();

                long t_start = SystemClock.uptimeMillis();
                urlConnection.setDoOutput(true);
                urlConnection.setDoInput(true);
                urlConnection.setRequestProperty("Accept-Encoding", "identity");

                OutputStream out = new BufferedOutputStream(urlConnection.getOutputStream());

                writeRequestHeader(out);

                // Downsample and compress input to build proxy input
                long t_start_up = SystemClock.uptimeMillis();
                if(scaleFactor == 1){
                    Log.d("RecipeMobile", "Uploading full input to server");
                    unprocessed_image.compress(Bitmap.CompressFormat.JPEG, 95, out);
                } else {
                    int height                  = unprocessed_image.getHeight();
                    int width                   = unprocessed_image.getWidth();
                    Log.d("RecipeMobile", "Uploading degraded input to server ("+scaleFactor+","+inputQuality+")");
                    final Bitmap degraded_input = Bitmap.createScaledBitmap(unprocessed_image,width/scaleFactor,height/scaleFactor, false);
                    degraded_input.compress(Bitmap.CompressFormat.JPEG, inputQuality, out);
                }

                // Gather histograms for upload
                preprocessingThread.join();
                JNIRecipePtr = rPre.getJNIRecipePtr();
                if(rPre._histograms != null) {
                    out.write(rPre._histograms);
                }
                rPre = null;

                out.close();
                Log.d("RecipeMobile", "Upload time: "+(SystemClock.uptimeMillis()-t_start_up)+"ms");
                t_start = SystemClock.uptimeMillis();

                // Start precomputing the image features while we wait for the server
                Thread precomputationThread = new Thread(new Precomputation(JNIRecipePtr), "precomputation thread");
                precomputationThread.start();

                // Get response
                BufferedInputStream in = new BufferedInputStream(urlConnection.getInputStream());

                // Parse header
                Map<String,List<String>> header = urlConnection.getHeaderFields();

                // Unpack data
                byte[] buffer = new byte[4];

                // Number of bytes for the lowpass data
                in.read(buffer);
                ByteBuffer bb = ByteBuffer.wrap(buffer).order(ByteOrder.LITTLE_ENDIAN );
                int nLP = bb.getInt();

                // Number of bytes for the highpass data
                in.read(buffer);
                bb = ByteBuffer.wrap(buffer).order(ByteOrder.LITTLE_ENDIAN );
                int nHP = bb.getInt();

                // Number of bytes for the quantization table
                in.read(buffer);
                bb = ByteBuffer.wrap(buffer).order(ByteOrder.LITTLE_ENDIAN );
                int nFloats = bb.getInt();

                // Recipe lowpass and highpass
                recipe_lp = BitmapFactory.decodeStream(in);
                recipe_hp = BitmapFactory.decodeStream(in);

                // Quantization table
                byte[] bufferQtable = new byte[nFloats*4];
                in.read(bufferQtable);
                bb = ByteBuffer.wrap(bufferQtable).order(ByteOrder.LITTLE_ENDIAN );
                qtable = new float[nFloats];
                bb.asFloatBuffer().get(qtable);

                in.close();

                // Sync with feature pre-computation
                precomputationThread.join();

                long t_stop = SystemClock.uptimeMillis();
                Log.d("RecipeMobile", "Remote processing time: "+(t_stop-t_start)+"ms");

                // Finalize output reconstruction
                recipeReconstruct(recipe_hp, recipe_lp, qtable, processed_image, JNIRecipePtr);
                t_stop = SystemClock.uptimeMillis();

                // Cleanup
                destroyRecipeState(JNIRecipePtr);

                status_indicator.setText("Remote(recipe) processing in "+(t_stop-t_start_global)+"ms");
                updatePreview(processed_image);


            } catch(Exception e) {
                e.printStackTrace();
            } finally {
                urlConnection.disconnect();
            }

        } catch(Exception e) {
            e.printStackTrace();
        }
    }
}
