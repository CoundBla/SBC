/**Constantes de funcionamiento firebase */
const functions = require("firebase-functions");
const express = require("express");
// eslint-disable-next-line no-unused-vars
const cors = require("cors");
//firebase Admin
const admin = require("firebase-admin");

//const firestore =require("@google-cloud/firestore");
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
 *  id: ID del sd.
 *  tipo: "clase",
 *  aula2: aula secundaria en caso de que el aforo esté completo
 *  aforoMax: aforo máximo de la sala
 *  fecha_ini: string de la fecha y hora a la que empezará el "evento". formato: "aaaa/mm/dd hh:mm:ss"
 *  fecha_fin: string de la fecha y hora a la que terminará el "evento". formato: "aaaa/mm/dd hh:mm:ss"
 * Datos de salida:
*/
  DoorApp.post("/SetDoorConfiguration",async (req,res)=>{
    var mapToSend = {};
    var data = req.body;
    //TODO: Comprobar por auth si hay token y privilegios (participante)
    const doorRef = db.collection("Door").doc(data.id).collection("Evento");
    //TODO: Comprobar si existe evento para la misma fecha.


    //añadimos datos de la configuración
    mapToSend.tipo = data.tipo;
    mapToSend.Aula2 = data.aula2;
    mapToSend.Aforo = 0;
    mapToSend.AforoMax = data.aforoMax;
    mapToSend.fecha_ini = getFechaFromString(data.fecha_ini);
    mapToSend.fecha_fin = getFechaFromString(data.fecha_fin);

    await doorRef.add(mapToSend)
    .catch((err)=>{
        res.status(500).json({error:"Se ha producido un error al actualizar los datos de la puerta: "+err})
    })
    return res.status(200).json(mapToSend)    
  });
function getFechaFromString(fecha){
  return Math.round(new Date(fecha).getTime()/1000);
}  

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

    mapReturn.isValid = 0;
    if(validCodeDoc.exists){
      if(validCodeDoc.data().checkFl){
        mapReturn.isValid = 1;
      }  
    }
    return res.status(200).json(mapReturn);
  });  
  /**Función que configura los permisos de acceso de nfc
   * {
   *  id:"firebase_ID",
   *  matlist: "string de matriculas separado por ,"
   * }
   */
  DoorApp.post("/SetAccess", async(req,res)=>{
    
    var data = req.body;

    const validCodeRef = db.collection("Door").doc(data.id).collection("ValidNFC");
    const usersRef = db.collection("User");

    
    var matList = data.matlist.split(",");
    console.log(matList);
    var nfcList = [];
    for(var i=0; i<matList.length;i++)
    {
      var arrayUser = await usersRef.where("matricula","==",matList[i]).get();
      console.log(matList[i]);
      if(!arrayUser.empty){
        console.log("HAY ARRAYUSER");
        arrayUser.forEach(user=>{
          var data = user.data();
          console.log(data.nfc);
          nfcList.push(data.nfc);
        })
      }
    }

    var arrayValidNFC = await validCodeRef.get();
    
    const batch = db.batch();
    for(i=0;i<nfcList.length;i++)
    {            
      var existeNFC = false;
      for(var j=0;j<arrayValidNFC.length;j++){
        if(arrayValidNFC[j].id == nfcList[i]){
          existeNFC = true;
          break;
        }
      }
      var nfcRef = validCodeRef.doc(nfcList[i]);
      if(existeNFC){
        batch.update(nfcRef,{activeFl:"true"});
      }
      else{
        batch.set(nfcRef,{Checks:0,activeFl:"true"});
      }
    }
    var errMsg = await batch.commit(); 
    console.log(errMsg);
    return res.status(200).json({msg:"Se ha configurado el acceso a la sala"});
  });
/**
 * {
 *  id: DoorID,
 *  idEvento: eventoID,
 *  nfc: codNC
 * }
 */
  DoorApp.post("/SetNewCheck", async(req,res)=>{
    var mapReturn = {};
    var data = req.body;

    const eventoRef = db.collection("Door").doc(data.id).collection("Evento")
    var objLastEvento = await GetLastEvent(data.id);    
    var keys = Object.keys(objLastEvento);
    for(var i=0; i<keys.length;i++){      
      var mapdata = objLastEvento[keys[i]];
      mapdata.Aforo++;
      mapReturn = await eventoRef.doc(keys[i]).update(mapdata);
    }

    return res.status(200).json(mapReturn);
  });

  DoorApp.post("/getLastEvent",async(req,res)=>{
    var mapReturn = {};
    var data = req.body;

    mapReturn = await GetLastEvent(data.id);

    return res.status(200).json(mapReturn);
  });

  async function GetLastEvent(doorID){
    const eventRef = db.collection("Door").doc(doorID).collection("Evento");
    var mapReturn = {};
    var evento = await eventRef.orderBy("fecha_ini","desc").limit(1).get()

    evento.forEach(ev=>{
      
      mapReturn[ev.id] = ev.data();      
    })
    console.log(mapReturn);
    return mapReturn;

  }
  /************************************
 ***EXPORTAMOS FUNCIONES DE EducaGame***
 ************************************/

exports.door = functions
  .region("europe-west1")
  .https.onRequest(DoorApp);
