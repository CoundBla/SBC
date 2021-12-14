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
  const DoorApp = express();
/**Función que devuelve el valor de la paga semanal (acumulado) y si se ha cobrado 
 * Datos de entrada:
 * Datos de salida:
*/
  DoorApp.post("/SetDoorConfiguration",async (req,res)=>{
    var mapToSend = {};
    var data = req.body;
    //TODO: Comprobar por auth si hay token y privilegios (participante)
    const doorRef = db.collection("door").doc(data.did);
   
    mapToSend.Aula = data.aula;
    mapToSend.Aforo = data.Aforo;
    mapToSend.AforoMax = data.maxAforo;

    await doorRef.update(mapToSend)
    .catch((err)=>{
        res.status(500).json({error:"Se ha producido un error al actualizar los datos de la puerta: "+err})
    })    
    return res.status(200).json(mapToSend)
    
  });
/**Función que comprueba si el código pasado por el lector es válido.
 * {
 *  id: "firebase_ID",
 *  nfcID: "urlCaracteres"
 * }
 */
  DoorApp.post("/IsValidCode", async(req,res)=>{
    var mapReturn = {};
    var data = req.body;

    const validCodeRef = db.collection("Door").doc(data.id).collection("ValidNFC").doc(data.nfcID);
    const validCodeDoc = await validCodeRef.get();
    
    if(!validCodeDoc.exists){
      mapReturn.isValid = 0;
    }
    else{
      mapReturn.isValid = 1;
    }
    return res.status(200).json(mapReturn);
  });
  

  /************************************
 ***EXPORTAMOS FUNCIONES DE EducaGame***
 ************************************/

exports.door = functions
  .region("europe-west1")
  .https.onRequest(DoorApp);
