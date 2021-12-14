/**Constantes de funcionamiento firebase */
const functions = require("firebase-functions");
const express = require("express");
// eslint-disable-next-line no-unused-vars
const cors = require("cors");
//firebase Admin
const admin = require("firebase-admin");
/**Definimos las constantes de la app */

//Inicializamos App si hace falta
if(admin.apps.length === 0) {
    admin.initializeApp();
  }
/**API*/
  const db = admin.firestore();
  const FirmWareApp = express();
/**Función que devuelve el valor de la paga semanal (acumulado) y si se ha cobrado 
 * Datos de entrada:
 * Datos de salida:
*/
  FirmWareApp.get("/GetLastFirmWare",async (req,res)=>{    
    var mapReturn = {};
    //TODO: Comprobar por auth si hay token y privilegios (participante)
    const firmwareRef = db.collection("FirmWare");
    const lastFirmWare = await firmwareRef.orderBy("Version","desc").limit(1).get();
    lastFirmWare.forEach(doc => {
        mapReturn[doc.id] = doc.data();
    });
    return res.status(200).json(mapReturn);
  });
  
  /*FirmWareApp.post("/SetLastUpdate",async(req,res)=>{
    var mapReturn = {};
    var data = req.body;
    const firmwareUpdateRef = db.collection("Door").doc(data.doorId).collection("FirmWareUpdateLog");
    
  });*/
  /************************************
 ***EXPORTAMOS FUNCIONES DE EducaGame***
 ************************************/

exports.firmware = functions
  .region("europe-west1")
  .https.onRequest(FirmWareApp);
