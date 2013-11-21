/* -------------------------------------------
  Node Web server for the FapCarDash
  
  ver 0.1    -  23/10/2013
  Auth. A.Franco
  
  History 


--------------------------------------------- */
// define constants
var isDebug = true;
var port = 8000;
var indexPage = 'fapcardashfe.html'; // the main page splashscreen
var localBaseDir = __dirname + '/Web/' ;
var interProcessExchangeFile = __dirname + '/exchange.xml';
var itemsXMLFileDescriptor = __dirname + '/itemsDefinition.xml';


// define globals variables
var webServer = require('http').createServer(onRequest),
  ioSock = require('socket.io').listen(webServer),
  parser = new require('xml2json'),
  fs = require('fs'),
  url = require("url");

var redis = require("redis");
// --->var controlIOSocket; // This is the io socket that partecipate the game
var subscribedSockets = [];

var xml2object = require('xml2object');


// creating the server 
webServer.listen(port);

// -----  connect with the Redis DB ---------
redisPushClient = redis.createClient();

// ----- subscribe to DB items
subscribeToPushServer(itemsXMLFileDescriptor);

// -------------  End of main program -----------------------------



// Function that react request from web client
function onRequest(req, res) {

	// analyze the request 
	var pathName = url.parse(req.url).pathname;
	if(isDebug) console.log(' onRequest ::= Request url:' + pathName +'\n');
	var query = url.parse(req.url).query;
//	if(isDebug) console.log(' onRequest ::= Request query:' + query +'\n');
	var hash = url.parse(req.url).hash;
//	if(isDebug) console.log(' onRequest ::= Request hash:' + hash +'\n');
  
	var err;

	// correct the null URL
	pathName = ( pathName == '/'  ? indexPage : pathName );

	// if the page have a query string process it, now ignore
	// if(query != '') pathName = DoSomeThing(pathName);
  
	// if the page have a hash string process it, now ignore
	// if(hash != '') pathName = DoSomeThing(pathName);

	// Now we can read the file and send it ...
	fs.readFile(localBaseDir + pathName, function(err, data) {

		if (err) {
    		console.log(" onRequest :: Error access the file:"+pathName);
			console.log(err);     
			res.writeHead(500);
			res.end('Error loading the file '+ pathName);  // terminate with error
			}
		else {
			res.writeHead(200);
			res.end(data);
			}
//		if(isDebug) console.log(' onRequest ::= exit function \n');
    	return;
    	});
	}
// ------------------------------------


//------------------------------------
// WEB SOCKET COMUNICATION 
//------------------------------------
ioSock.sockets.on('disconnect', function(socket) {

	var index = subscribedSockets.indexOf(socket);
	if (index > -1) {
		subscribedSockets.splice(index, 1);
		}
	});


// creating a new websocket to keep the content updated
ioSock.sockets.on('connection', function(socket) {

	// first of all create a socket for the interprocess exchange 
	var subscribedSocketType = "";
	var controlIOSocket = socket;
	if(isDebug) console.log("ioSock connection ::= Create the I/O socket ");

	// Handler to manage the parameters request
	controlIOSocket.on('subscribe', function (type) {
		if(type != "") {
			subscribedSocketType = type;
			subscribedSockets.push(controlIOSocket);
			}
		});

	// Handler to manage the parameters request
	controlIOSocket.on('getparams', function (filter) {
		if(filter == 'maps') { // retrive params for the mapping
			var paramNames = new Array("isFixed","pointsOfInterest");
			__readArrayFromDB(paramNames, controlIOSocket);
			}
		if(filter == 'setup') { // retrive params for the setup
			var paramNames = new Array("AlarmRoll","AlarmPitch","BaseAltimeter","Meteo1Name","Meteo1Id","Meteo2Name","Meteo2Id","Meteo3Name","Meteo3Id","Meteo4Name","Meteo4Id","Meteo5Name","Meteo5Id");
			__readArrayFromDB(paramNames, controlIOSocket);				
			}
		});
	
	// Handler to manage one parameter request
	controlIOSocket.on('getparam', function (jsonString) {
		var parameter = JSON.parse(jsonString);
		if(isDebug) console.log("ioSock connection ::= Request parameter value for : %s ", parameter.$name);
		// Create the Redis client connection
		redisGetClient = redis.createClient();
		redisGetClient.get(parameter.name, function (err, value) {
		    if (err) throw err;
			var item = new Object();
			item.name = parameter.name;
			item.value = value;
			if( controlIOSocket != null) {
				var jsonString = JSON.stringify(item);
			 	controlIOSocket.volatile.emit('parameter', jsonString);
			 	}   
			});
		// Shut down the temporary get redis client
		redisGetClient.quit();
		});

	// Hanler to react to a save param request
	controlIOSocket.on('setparam', function (jsonString) {
		var parameter = JSON.parse(jsonString);
		if(isDebug) console.log("ioSock connection ::= Setting parameter value for %s: ", parameter.name);
		// Create the Redis client connection
		redisSetClient = redis.createClient();
		// Set the value
		redisSetClient.set(parameter.name, parameter.value);
		// Shut down the temporary get redis client
		redisGetClient.quit();
		return;
		});
	
	});

// internal function for read Lists from Redis 
function __readListOfObjectsFromDB(listName, controlIOSocket) {
	// Create the Redis client connection
	redisGetClient = redis.createClient();
	var arrayOfPoi = [];
	redisGetClient.lrange(listName, 0, -1, function (error, poiList) {
		if (error) throw error
		poiList.forEach(function (item) {  // each item is a JSON string
			var poi = new Object();
			poi = JSON.parse(item);
			arrayOfPoi.push(poi);
			})
		// Now we format the JSON string for the delivery
		if( controlIOSocket != null) {
			var JSONstring = JSON.stringify({paramArray: arrayOfPoi});
			controlIOSocket.volatile.emit('listofobjects', JSONstring);
			}
		// Shut down the temporary get redis client
		redisGetClient.quit();
		});
	}
// read from the DB an array of parmeters
function __readArrayFromDB(paramNames, controlIOSocket) {
	// Create the Redis client connection
	redisGetClient = redis.createClient();
	paramValues = [];
	for(var i=0;i<paramNames.length;i++){
		redisGetClient.get(paramNames[i], function (err, value) {
		    if (err) throw err;
		    paramValues.push(value);  // store the value into the array
			if (paramValues.length == paramNames.length) {  // When the last callback fun is called
				var objectsArray = new Array();
				for(var j=0;j<paramNames.length;j++){
					Param = new Object();  // Build the Array of parameter objects
					Param.name = paramNames[j];
					Param.value = paramValues[j];
					objectsArray.push(Param);
					}
				// Now we format the JSON string for the delivery
				if( controlIOSocket != null) {
					var JSONstring = JSON.stringify({paramArray: objectsArray});
					controlIOSocket.volatile.emit('parameters', JSONstring);
					}
				// Shut down the temporary get redis client
				redisGetClient.quit();
				}
			});
		}
	return;
	}
// --------------------------------

//------------------------------------
//  REDIS DB COMUNICATION 
//------------------------------------

// define the function that react to messages... 
redisPushClient.on("message", function (channel, message) {
	var item = new Object();
	item.channel = channel;
	item.message = message;
	if(isDebug) console.log("redisPushClient ::= recieve message %s", channel);

    var jsonString = JSON.stringify(item);
    for(var i=0; i<subscribedSockets.length; i++){
    	if( subscribedSockets[i] != null) {
    		subscribedSockets[i].emit('notification', jsonString);
	 		}
    	}
    });

redisPushClient.on("subscribe", function (channel,count) {
	if(isDebug) console.log("RedisClient ::= subscribed >" + channel + " -- " + count);
	return;
	});
// --------------------------------------
// Subscribe all the Items defined into the XML file
function subscribeToPushServer(fileName) {
	var parser = new xml2object([ 'item' ], fileName);
	//Bind to the object event to work with the objects found in the XML file
	parser.on('object', function(name, obj) {
		if(isDebug) console.log("subscribeToPushServer ::= Found an object: %s", obj.$t);
		redisPushClient.subscribe(obj.$t);
	});
	//Bind to the file end event to tell when the file is done being streamed
	parser.on('end', function() {
		if(isDebug) console.log("subscribeToPushServer ::= Connect & Subscribe to the Redis Server");
	});	
	//Start parsing the XML
	parser.start();
	return;
	}
// -------------  EOF ---------------------------------

