var AlexaAppServer = require('alexa-app-server');
var server = new AlexaAppServer({ port: 80, debug: false });
server.start();
server.express.use('/test', function(req, res) { res.send("OK"); });