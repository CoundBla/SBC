const functions = require("firebase-functions");
const express = require("express");
// eslint-disable-next-line no-unused-vars
const cors = require("cors");

const admin = require("firebase-admin");
//const { user } = require("firebase-functions/lib/providers/auth");
if(admin.apps.length === 0) {
  admin.initializeApp();
}
const auth = admin.auth();

const db = admin.firestore();
const UserApp = express();

UserApp.use(cors({origin: true}));

/**Función para crear un nuevo usuario por email y contraseña. al crear el usuario auth, se creará un registro user en firebase
 * Además se crearán las misiones obligatorias y se asignaran al usuario.
 */
UserApp.post("/SetNewUser", async(req, res) => {
    const userMap = req.body;
    var errorMsg = {}, credenciales;
    errorMsg = validateUser(userMap);
    if(errorMsg)
    {
        return res.status(500).json(errorMsg);        
    }    
    try{
        await auth.createUser({
            email: userMap.Email,
            password: userMap.Password,
            displayName: userMap.NickName
        }).then((cred)=>{
            credenciales = cred;
        })
        .catch((err) => {//Si hay algún error al crear el usuario, devolvemos el error.
            console.log(err)
            return res.status(500).json({message: "error al crear usuario<br>" + err});
        });
        
        //Eliminamos el password ya que no es un dato que se pueda consultar desde firebase               
        delete userMap.Password;
        //Añadimos el mapa de atributos
        let partAttributes = require("./../Tipologías/PartAttributes");        
        userMap.Atributos = partAttributes.getInitializedAtributos();
        errorMsg = await db.collection("user").doc(credenciales.uid).set(userMap)            
            .catch((error) =>{
                console.log(error)
                return res.status(500).json({error:"Se ha producido un error al crear el registro de usuario: "+error});
            });
        //Para añadir la información a UsuarioMision, no hace falta el email del usuario
        delete userMap.Email;
        //await CrearAdministraciónUsuario(uid);//TODO: ver como implementar administracion de perfil.    
        if(userMap.TipoUsuario === "P"){            
            try {
                await createMandatoryQuest(credenciales)//TODO: Revisar por qué no funciona.                
                .then(errorMsg=>{
                    if(errorMsg && Object.prototype.hasOwnProperty.call(errorMsg,"error")){                        
                        errorMsg.message = "usuario creado correctamente";                        
                        return res.status(500).json({error: errorMsg});
                    }                    
                })
                .catch((err)=>{
                    console.log("ERROR")
                    console.log(err)
                    return err;
                });
                await createInventory(credenciales.uid)
                .then(errorMsg=>{
                    if(errorMsg && Object.prototype.hasOwnProperty.call(errorMsg,"error")){                        
                        errorMsg.message = "usuario creado correctamente";                        
                        return res.status(500).json({error: errorMsg});
                    }                    

                })
                await createMisionEnCurso(credenciales.uid)
                .then(errorMsg=>{
                    if(errorMsg && Object.prototype.hasOwnProperty.call(errorMsg,"error")){                        
                        errorMsg.message = "usuario creado correctamente";                        
                        return res.status(500).json({error: errorMsg});
                    }                    
                    else{
                        return res.status(200).json({message:"usuario creado correctamente"});
                    }
                })
            } 
            catch (error) {
                console.log(error)
                return res.status(500).json({error:error.message});
            }
        }
        else{
            res.status(200).json({message:"Supervisor creado correctamente"});
        }
    }
    catch(error){
        console.log(error)
        return res.status(500).json({error:error.message});
    }
});
function validateUser(user)
{
    if(!user.NickName || user.NickName.trim() === "")
    {
        return {message:"Debe rellenar el Nickname"};
    }
    if(!user.Email || user.Email.trim() === "")
    {
        return {message:"Email es obligatorio"};
    }
    if(!user.Password || user.Password.trim() === "")
    {
        //Faltan reglas. minimo x caracteres.
        return {message:"La contraseña es obligatoria"};
    }

}
/*async function CrearAdministraciónUsuario(uid){
    var perfilInfo, adminInfo, docUserRef;
    
    docUserRef = db.collection("user").doc(uid);
    //TODO: Crear reglas de acceso. Admite lectura por usuario, escritura solo admin.
    perfilInfo = await docUserRef.collection("").doc("ss").set(uid);
    //TODO: Crear reglas de acceso. Solo admite escritura de admin. lectura libre.
    
}*/
async function createMandatoryQuest(credenciales){    
    var misionFunc = require("./Misiones");
    
    try{
        var arrayError = [];
        //Creamos la consulta para traer las misiones obligatorias activas
        var MisionesDiarias = await db.collection("MisionDiariaAdmin")
            .where("TipoMision","==","O")
            .where("ActiveFl","==",true)
            .get();
        if(MisionesDiarias.empty){
            return {error:"No se han encontrado misiones diarias Obligatorias"};
        }
        /**Por cada mision obtenida, creamos un registro en misiondiaria/{misionID}/Participante
         * Además se crea un registro en UsuarioMision/{usuarioID}/MisionDiaria
         * Se utilizará misionFunc para esto */                 
        //TODO: https://stackoverflow.com/questions/37576685/using-async-await-with-a-foreach-loop        
        MisionesDiarias.forEach(async misionDoc => {            
            let misionRef = db.collection("MisionDiariaAdmin").doc(misionDoc.id);
            const userDoc = await db.collection("user").doc(credenciales.uid).get()
            return misionFunc.setParticipanteMision(userDoc, misionDoc, misionRef)
                .then(()=>{
                    return misionFunc.setUsuarioMision(userDoc,misionDoc,false)
                    .catch((error)=>{
                        let errorMsg ="<br> Fallo al crear datos de la mision {"+misionDoc.id+"} en \"UsuarioMision/"+userDoc.id+"\"| error:"+error;
                        console.log(errorMsg)
                        arrayError.push(errorMsg)
                        return;
                    })                            
                })
                .catch((error)=>{
                    let errorMsg ="<br> Fallo al crear datos de asignación de la mision - "+misionDoc.id+"| error: "+error;
                    console.log(errorMsg)
                    arrayError.push(errorMsg);
                    return;
                })                    
        })
                   
    }
    catch(err){
        console.log("error:"+err)
        return {error: "Se ha producido un error: "+err};
    }    
    if(arrayError.length){
        let errores = arrayError.join("<br>");
        arrayError = [];
        arrayError.push("Se han producido los siguientes errores: ");
        arrayError.push(errores)
        return {error: arrayError.join("<br>")};
    }    
//TODO: Las misiones en curso se sobreescriben debido a que el user no actualiza. Revisar para utilizar el campo misionesEnCurso.
//Por el momento, en la Fase2 el front accederá directamente a usuario mision para traer las misiones en curso.
}
/**Función para crear el inventario al crear el usuario */
async function createInventory(userId){
    var Monedas, monedaMain, mapMonedas, strMonedaMainAcumulada;
    const inventoryRef = db.collection("user").doc(userId).collection("Inventario");
    const iMonedaRef = inventoryRef.doc("Monedas");    

    Monedas = require("./Monedas");
    monedaMain = Monedas.MonedaMain;
    mapMonedas = {};
    mapMonedas[monedaMain.codigo]= 0;
    strMonedaMainAcumulada = monedaMain.codigo+"_Acumulados";
    mapMonedas[strMonedaMainAcumulada] = 0;
    await iMonedaRef.set(mapMonedas)
        .catch((err)=>{
            return {error: "Se ha producido un error al crear la moneda principal. "+err};
        })    
}
/**Función para crear las misiones obligatorias que estén en curso*/
async function createMisionEnCurso(userId){
    var userDoc, mapMisionEnCurso = {};
    const misionDiariasObligatorias = await db.collection("MisionDiariaAdmin").where("TipoMision","==","O")
        .where("InitialDate","<=",new Date()).get()
    const userRef = db.collection("user").doc(userId)
    userDoc = await userRef.get()

    if(!userDoc.exists){
        return {error:"No se ha encontrado el usuario"}
    }
    if(misionDiariasObligatorias.empty){
        return {error:"No se han encontrados misiones obligatorias"}
    }
    misionDiariasObligatorias.forEach(mision=>{
        mapMisionEnCurso[mision.id] = mision.data().Titulo
    })
    return userRef.update({MisionesEnCurso:mapMisionEnCurso})    
    .catch((err)=>{
        return {error:"Se ha producido un problema al actualizar el usuario. "+err}
    })
}

/*************************************/
/***EXPORTAMOS FUNCIONES DE USUARIOS**/
/*********************************** */
exports.user = functions
    .region("europe-west1")
    .https.onRequest(UserApp);