/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package dbinterface;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.URL;
import java.net.URLConnection;
import java.util.ResourceBundle;
import java.util.logging.Level;
import java.util.logging.Logger;
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.fxml.Initializable;
import javafx.scene.CacheHint;
import javafx.scene.chart.LineChart;
import javafx.scene.chart.XYChart;
import javafx.scene.control.Button;
import javafx.scene.control.Label;
import javafx.scene.image.Image;
import javafx.scene.image.ImageView;

/**
 *
 * @author Mag
 */
public class FXMLDocumentController implements Initializable {
    
    @FXML
    private Label WaterLabel;
    
    
    
    @FXML
    private LineChart<Number,Number> graphLabel;
    
    
     
    @FXML
    private Button Refbutton ;
    
    @FXML
    private ImageView blomma ;
    
    
   
     
    
    
    
    
    
    
    
    @FXML
    private void handleButtonAction(ActionEvent event) throws IOException{
   

        
                int WL = getWaterLevel(getRows()-1);
        
               WaterLabel.setText(WL + "");
               
      
        
    }
    
    
    
    
    
    @Override
    public void initialize(URL url, ResourceBundle rb) {
       
        
     
        
        
        int rows = 0;
        try {
            // TODO
            rows = getRows();
            
        } catch (IOException ex) {
            Logger.getLogger(FXMLDocumentController.class.getName()).log(Level.SEVERE, null, ex);
        }
        
        XYChart.Series<Number,Number> series = new XYChart.Series<>();
        
        if(rows>0){
        for(int L=rows-11;L<rows;L++){
            try {
                series.getData().add(new XYChart.Data(L-rows+11,getHumidity(L)));
            } catch (IOException ex) {
                Logger.getLogger(FXMLDocumentController.class.getName()).log(Level.SEVERE, null, ex);
            }
        }
        
            }
        graphLabel.getData().add(series);
        
        
    }   
    
    
    private int getHumidity(int id) throws IOException {
        String page="http://ifp.hopto.org/testserver/dataOut.php/?id=";
        page += id;
      URL oracle = new URL(page);
        URLConnection yc = oracle.openConnection();
        try (BufferedReader in = new BufferedReader(new InputStreamReader(
                yc.getInputStream()))) {
            String inputLine;
            
            String buff = "";
            if((inputLine = in.readLine()) != null){
                buff = inputLine;
            }
            
            while ((inputLine = in.readLine()) != null){}
        String[] buff2 = buff.split(" ");
        return Integer.parseInt(buff2[2]);
    }}
    
     private int getWaterLevel(int id) throws IOException {
        String page="http://ifp.hopto.org/testserver/dataOut.php/?id=";
        page += id;
      URL oracle = new URL(page);
        URLConnection yc = oracle.openConnection();
        try (BufferedReader in = new BufferedReader(new InputStreamReader(
                yc.getInputStream()))) {
            String inputLine;
            
            String buff = "";
            if((inputLine = in.readLine()) != null){
                buff = inputLine;
            }
            
            while ((inputLine = in.readLine()) != null){}
        String[] buff2 = buff.split(" ");
        return Integer.parseInt(buff2[3]);
    }}
    
    private int getRows() throws IOException {
        String page="http://ifp.hopto.org/testserver/tableSum.php";
        
      URL oracle = new URL(page);
        URLConnection yc = oracle.openConnection();
        try (BufferedReader in = new BufferedReader(new InputStreamReader(
                yc.getInputStream()))) {
            String inputLine;
            
            String buff = "";
            if((inputLine = in.readLine()) != null){
                buff = inputLine;
            }
            
            while ((inputLine = in.readLine()) != null){}
        String[] buff2 = buff.split(" ");
        return Integer.parseInt(buff2[0]);
    }}
    
    
}
