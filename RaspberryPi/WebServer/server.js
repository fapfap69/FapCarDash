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
var localBaseDir = __dirname + '/Web' ;
var interProcessExchangeFile = __dirname + '/exchange.xml';
var itemsXMLFileDescriptor = __dirname + '/itemsDefinition.xml';


// define globals variables
var webServer = require('http').createServer(onRequest),
  ioSock = require('socket.io').listen(app),
  parser = new require('xml2json'),
  fs = require('fs'),
  url = require("url");

var redisClient = require("redis");
var controlIOSocket; // This is the io socket that partecipate the game

// creating the server 
webServer.listen(port);

// -----  connect with the Redis DB ---------
redisClient = redis.createClient();

// ----- subscribe to DB items
subscribeToPushServer(itemsXMLFileDescriptor);

// -------------  End of main program -----------------------------



// Function that react request from web client
function onRequest(req, res) {

	// analyze the request 
	var pathName = url.parse(req.url).pathname;
	if(isDebug) console.log(' onRequest ::= Request url:' + pathName +'\n');
	var query = url.parse(req.url).query;
	if(isDebug) console.log(' onRequest ::= Request query:' + query +'\n');
	var hash = url.parse(req.url).hash;
	if(isDebug) console.log(' onRequest ::= Request hash:' + hash +'\n');
  
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
    		console.log(" onRequest :: Error access the file:'+pathName "
			console.log(err);     
			res.writeHead(500);
			res.end('Error loading the file '+pothName);  // terminate with error
			}
		else {
			res.writeHead(200);
			res.end(data);
			}
		if(isDebug) console.log(' onRequest ::= exit function \n');
    	return;
    	});
	}
// ------------------------------------

// creating a new websocket to keep the content updated
ioSock.sockets.on('connection', function(socket) {

	// first of all create a socket for the interprocess exchange 
	controlIOSocket = socket;
	if(isDebug) console.log("ioSock connection ::= Create the I/O socket ");

	// seconds hit a function for the file change polling mechanism (this is an extra)
	if(interProcessExchangeFile != '') {
	  	if(isDebug) console.log('ioSock connection ::= Watch the file: ' + interProcessExchangeFile);
  		fs.exists(interProcessExchangeFile, function (exists) {
  			if ( !exists) {
  				if(isDebug) console.log('ioSock connection ::= The file to Watch not exists ! - ' + interProcessExchangeFile);
  				}
  			else {
				fs.watch(interProcessExchangeFile, onXMLfileChange);
  		 		}
  			});
  		}
	});
// --------------------------------

// watching the xml file changing   
function onXMLfileChange(curr, prev) {
    // on file change we can read the new xml
    fs.readFile(interProcessExchangeFile, function(err, data) {
	    if (err) throw err;
    	// parsing the new xml data and converting them into json file
    	var json = parser.toJson(data);
    	// adding the time of the last update
    	json.time = new Date();
    	// send the new data to the client
    	if( controlIOSocket != null) {
	 		controlIOSocket.volatile.emit('notification', json);
			}
		});
	}
// ------------------------------------------

// define the function that react to messages... 
redisClient.client1.on("message", function (channel, message) {

    if(isDebug) console.log("RedisClient ::= message from channel " + channel + ": " + message);

	var item = new Object();
	item.channel = channel;
	item.message = message;

    var jsonString = JSON.stringify(item);
	if( controlIOSocket != null) {
	 	controlIOSocket.volatile.emit('notification', jsonString);
	 	}
    });

redisClient.on('subscribe', function (channel,count) {
	if(isDebug) console.log("RedisClient ::= subscribed >" + channel + " -- " + count);
	});
// --------------------------------------------

// Subscribe all the Items defined into the XML file
function subscribeToPushServer(fileName) {
	//redisClient.incr("pip e peppo ");
	
    fs.readFile(fileName, function(err, data) {
	    if (err) throw err;
    	// parsing the xml items definition and converting them into js Object
    	Object itemsList = parser.toJson(data, {object:true});
		var key;
		
		for (key in itemsList) {
        	if (itemsList.hasOwnProperty(key)) {
        		redisClient.subscribe(itemsList[key]);
        		};
    		}
		});
	if(isDebug) console.log("subscribeToPushServer ::= Connect & Subscribe to the Redis Server");
	}
// ---------------------------------------------
	
	
// -------------  EOF ---------------------------------

