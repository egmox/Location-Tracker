package mx.egm.location;
import android.os.AsyncTask;
import android.os.Handler;
import android.support.v4.app.FragmentActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.Toast;

import com.google.android.gms.maps.CameraUpdateFactory;
import com.google.android.gms.maps.GoogleMap;
import com.google.android.gms.maps.OnMapReadyCallback;
import com.google.android.gms.maps.SupportMapFragment;
import com.google.android.gms.maps.model.LatLng;
import com.google.android.gms.maps.model.Marker;
import com.google.android.gms.maps.model.MarkerOptions;

import org.apache.http.HttpResponse;
import org.apache.http.NameValuePair;
import org.apache.http.client.HttpClient;
import org.apache.http.client.entity.UrlEncodedFormEntity;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.DefaultHttpClient;
import org.apache.http.message.BasicNameValuePair;
import org.apache.http.util.EntityUtils;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.List;

public class MainActivity extends FragmentActivity implements OnMapReadyCallback {
    private GoogleMap mMap;
    float lat,lng;
    JSONObject json;
    SupportMapFragment mapFragment;
    LatLng bus718point;
    MarkerOptions bus718marker;
    Marker m,n;
    boolean first=true;
    int i=0;

    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        // Obtain the SupportMapFragment and get notified when the map is ready to be used.
        mapFragment = (SupportMapFragment) getSupportFragmentManager()
                .findFragmentById(R.id.map);

        new Handler().postDelayed(new Runnable() {
            @Override
            public void run() {
                new AsyncTask<Void,Void,Void>() {
                    protected Void doInBackground(Void... params){
                        try{
                            HttpClient httpclient= new DefaultHttpClient();
                            HttpPost httppost= new HttpPost("http://162.144.48.118/gps/zResponse.php");
                            List<NameValuePair> nameValuePairs = new ArrayList<NameValuePair>();
                            nameValuePairs.add(new BasicNameValuePair("device_id", "3"));
                            httppost.setEntity(new UrlEncodedFormEntity(nameValuePairs));
                            HttpResponse response= httpclient.execute(httppost);
                            if(response!=null){}
                            json = new JSONObject((EntityUtils.toString(response.getEntity(), "UTF-8")));
                        }
                        catch(Exception e){Log.e("log_tag", "Error parsing data " + e.toString());}
                        return null;}
                    protected void onPostExecute(Void result) {
                        super.onPostExecute(result);
                        try {
                            update();
                            lat = Float.valueOf(json.getString("lat"));
                            lng = Float.valueOf(json.getString("lng"));
                            bus718point = new LatLng(lat,lng);

                            bus718marker = new MarkerOptions().position(bus718point);
                        }catch(Exception e){Log.e("error","error is "+e.getMessage());}
                    }
                }.execute();
            }
        }, 3000);

    }

    public void onMapReady(GoogleMap googleMap) {

        mMap = googleMap;
        mMap.animateCamera(CameraUpdateFactory.zoomTo(30));
        if(first){
        n=mMap.addMarker(bus718marker);
            n.setTitle("718");
            Toast.makeText(this,"first",Toast.LENGTH_SHORT).show();
        }
        else{
        n.setPosition(bus718point);

            Toast.makeText(this,"second"+i++,Toast.LENGTH_SHORT).show();
        }


        //breakmarker = mMap.addMarker(breakmarker);
        //mMap.addMarker(new MarkerOptions().position(busbreak).title("Break Down"));
        //mMap.addMarker(new MarkerOptions().position(bus718).title("718")).showInfoWindow();
        //mMap.addMarker(new MarkerOptions().position(new LatLng(lat, lng)).title("718")).showInfoWindow();
        mMap.setMyLocationEnabled(true);
    }
    public void update(){
        mapFragment.getMapAsync(this);

    }
}