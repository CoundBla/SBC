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
UserApp.post("/SetDoor", async(req, res) => {
    const doorMap = req.body;
    var errorMsg = {}, credenciales;
    errorMsg = validateDoor(doorMap);
    if(errorMsg)
    {
        return res.status(500).json(errorMsg);
    }    
    try{
        await auth.createUser({
            email: doorMap.Email,
            password: doorMap.Password,
            displayName: doorMap.DoorCode
        }).then((cred)=>{
            credenciales = cred;
        })
        .catch((err) => {//Si hay algún error al crear el usuario, devolvemos el error.
            console.log(err)
            return res.status(500).json({message: "error al crear usuario<br>" + err});
        });
        
        //Eliminamos el password ya que no es un dato que se pueda consultar desde firebase               
        delete doorMap.Password;
        //Añadimos el mapa de atributos        
        errorMsg = await db.collection("Door").doc(credenciales.uid).set(doorMap)            
            .catch((error) =>{
                console.log(error)
                return res.status(500).json({error:"Se ha producido un error al crear el registro de usuario: "+error});
            });
        //Para añadir la información a UsuarioMision, no hace falta el email del usuario
        delete doorMap.Email;
        //await CrearAdministraciónUsuario(uid);//TODO: ver como implementar administracion de perfil.    
       
        res.status(200).json({message:"Supervisor creado correctamente"});
       
    }
    catch(error){
        console.log(error)
        return res.status(500).json({error:error.message});
    }
});
function validateDoor(door)
{
    if(!door.DoorCode || door.DoorCode.trim() === "")
    {
        return {message:"Debe rellenar el Nickname"};
    }
    if(!door.Email || door.Email.trim() === "")
    {
        return {message:"Email es obligatorio"};
    }
    if(!door.Password || door.Password.trim() === "")
    {
        //Faltan reglas. minimo x caracteres.
        return {message:"La contraseña es obligatoria"};
    }

}

/**Función para crear un nuevo usuario por email y contraseña. al crear el usuario auth, se creará un registro user en firebase
 * Además se crearán las misiones obligatorias y se asignaran al usuario.
 * usuario:
 * {
 *      Email: "ejemplo@ej.com",
 *      Password: "paswd",
 *      matricula: "codigo",
 *      mombre: "Nombre",
 *      apellidos: "Apellido1 Apellido2",
 *      nfc:"codigonfc"
 * }
 */
 UserApp.post("/SetUser", async(req, res) => {
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
            displayName: userMap.matricula
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
        errorMsg = await db.collection("User").doc(credenciales.uid).set(userMap)            
            .catch((error) =>{
                console.log(error)
                return res.status(500).json({error:"Se ha producido un error al crear el registro de usuario: "+error});
            });        
    }
    catch(error){
        console.log(error)
        return res.status(500).json({error:error.message});
    }
    return res.status(200).json({msg:"Usuario creado correctamente"});
});
function validateUser(user)
{
    if(!user.matricula || user.matricula.trim() === "")
    {
        return {message:"Debe rellenar el código de matrícula"};
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
/**Función que devuelve los usuarios dados de alta en el sistema */
//Esta función no tiene seguridad. TODO: Volver post, y que reciba como dato un password.
UserApp.get("/GetUsers", async(req,res)=>{
    var mapUsers = {};
    const userRef = db.collection("User");

    const listUsers = await userRef.orderBy("matricula","asc").get();
    if(listUsers.empty){
        return res.status(500).json({error:"No se han encontrado usuarios"});
    }
    listUsers.forEach(user=>{
        //var objusr = {};
        var usrData = user.data();
        mapUsers[usrData.matricula] = usrData.nombre +" "+ usrData.apellidos;        
    })
    return res.status(200).json(mapUsers); 
});
/*************************************/
/***EXPORTAMOS FUNCIONES DE USUARIOS**/
/*********************************** */
exports.user = functions
    .region("europe-west1")
    .https.onRequest(UserApp);