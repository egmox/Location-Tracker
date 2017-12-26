package mx.egm.location;

        import android.app.ProgressDialog;
        import android.content.Context;
        import android.os.AsyncTask;
        import android.widget.Toast;
        import org.apache.http.HttpResponse;
        import org.apache.http.NameValuePair;
        import org.apache.http.client.ClientProtocolException;
        import org.apache.http.client.entity.UrlEncodedFormEntity;
        import org.apache.http.client.methods.HttpPost;
        import org.apache.http.impl.client.DefaultHttpClient;
        import org.apache.http.message.BasicNameValuePair;
        import org.apache.http.util.EntityUtils;
        import org.json.JSONArray;
        import org.json.JSONObject;
        import java.io.BufferedInputStream;
        import java.io.File;
        import java.io.FileOutputStream;
        import java.io.IOException;
        import java.io.InputStream;
        import java.io.OutputStream;
        import java.io.UnsupportedEncodingException;
        import java.net.URL;
        import java.net.URLConnection;
        import java.util.ArrayList;
        import java.util.List;

public class db extends AsyncTask<String[], String[], String[]>{
    public String[] data=null, name={"device_id"}, keys, ret={null,null,null,null};
    public String downloadname, downloadurl;
    public Context ctx;
    public ProgressDialog pd=null;
    String error="";
    int datatype;

    public db(String[] jsondata, String[] respKeys, Context act){
        ctx=act;
        data=jsondata;
        keys=respKeys;
        pd=new ProgressDialog(ctx);
        pd.setMessage("Connecting");
        pd.setCancelable(false);
        datatype=0;
    }//initialise variables fetched from main

    protected void onPreExecute(){pd.show();}

    protected String[] doInBackground(String[]... arg0) {
        switch (datatype) {
            case 0:
                HttpPost httppost = new HttpPost("http://162.144.48.118/gps/zResponse.php");
                JSONObject jsonobj = new JSONObject();
                List<NameValuePair> nameValuePairs = new ArrayList<NameValuePair>();
                try {
                    for (int i = 0; i <= name.length - 1; i++) jsonobj.put(name[i], data[i + 2]); //array starts from 0
                    nameValuePairs.add(new BasicNameValuePair("device_id", jsonobj.toString()));
                    httppost.setEntity(new UrlEncodedFormEntity(nameValuePairs));//preparing json data
                    HttpResponse response = new DefaultHttpClient().execute(httppost);//executing json post request
                    ret[1] = (EntityUtils.toString(response.getEntity(), "UTF-8"));
                    error = "Updating done";
                } catch (UnsupportedEncodingException e) {error = "Unsupported encoding found";}
                catch (ClientProtocolException e) {error = "Client protocol error";}
                catch (IOException e) {error = "Could not connect to server";}
                catch (Exception e) {error = "Unknown error: " + e.getMessage();}

                try {
                    JSONObject jsonRootObject = new JSONObject(ret[1]);
                    JSONArray jsonArray = jsonRootObject.optJSONArray("notices");
                    String[] getKeys=new String[keys.length];
                    for (int i = 0; i < jsonArray.length(); i++) {
                        JSONObject jsonObject = jsonArray.getJSONObject(i);
                        for(int count=0; count < keys.length; count++) {
                            getKeys[count] = jsonObject.optString(keys[count]).toString();
                        }
                        if(!(jsonObject.optString(keys[7]).toString().equals("text"))){
                            int byteCount;
                            try {
                                URL url = new URL(jsonObject.optString(keys[8]).toString());
                                URLConnection conection = url.openConnection();
                                conection.connect();
                                InputStream input = new BufferedInputStream(url.openStream(), 8192);
                                new File("/sdcard/.bits/notice/").mkdir();
                                OutputStream output = new FileOutputStream("/sdcard/.bits/notice/"
                                        +jsonObject.optString(keys[7]).toString());
                                byte data[] = new byte[1024];
                                while ((byteCount = input.read(data)) != -1) {
                                    output.write(data, 0, byteCount);
                                }
                                output.flush();
                                output.close();
                                input.close();
                            } catch (Exception e) {
                            }
                        }
                    }
                }catch(Exception e){}

                break;
            case 1:
                int byteCount;
                try {
                    URL url = new URL(downloadurl);
                    URLConnection conection = url.openConnection();
                    conection.connect();
                    InputStream input = new BufferedInputStream(url.openStream(), 8192);
                    new File("/sdcard/.bits/notice/").mkdir();
                    OutputStream output = new FileOutputStream("/sdcard/.bits/notice/"+downloadname);
                    byte data[] = new byte[1024];
                    while ((byteCount = input.read(data)) != -1) {
                        output.write(data, 0, byteCount);
                    }
                    output.flush();
                    output.close();
                    input.close();
                } catch (Exception e) {
                }
                break;
        }
        return null;
    }

    protected void onPostExecute(String[] Void) {
        super.onPostExecute(Void);
        pd.cancel();//connecting dialog doesn't end until connection process is done

        Toast.makeText(ctx, error, Toast.LENGTH_SHORT).show();
    }
}