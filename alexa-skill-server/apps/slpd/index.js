var alexa = require('alexa-app');
var mqtt = require('mqtt');
var client  = mqtt.connect('mqtt://broker.mqtt-dashboard.com');
// Allow this module to be reloaded by hotswap when changed
module.change_code = 1;

  client.on('connect', function () {
  client.publish('espslpd', 'Connected to SLPD')
})
// Define an alexa-app
var app = new alexa.app('slpd');
app.launch(function(req, res) {
  res.say("SLPD!");
});

app.intent('ChangeChannel', {
  "slots": {"CHANNEL": "AMAZON.NUMBER" },
  "utterances": ["{change channel to|change the channel to|switch channel to|switch the channel to|channel}{1-100|CHANNEL}"]
}, function(req, res) {
  client.publish('espslpd', 'channel ' + req.slot('CHANNEL'));
  res.say('Changing channel to ' + req.slot('CHANNEL'));
console.log(req.data);
});
app.intent('ChangeVolume', {
  "slots": {"VOLUME": "AMAZON.NUMBER" },
  "utterances": ["{change volume to|bring volume to|volume}{1-100|VOLUME}"]
}, function(req, res) {
  client.publish('espslpd', 'volume ' + req.slot('VOLUME'));
  res.say('Changing volume to ' + req.slot('VOLUME'));
console.log(req.data);
});
app.intent('TurnOnOff', {
  "slots": {"STATUS": "ON_OFF" },
  "utterances": ["{turn it|turn T.V.}{On|Off}"]
}, function(req, res) {
  client.publish('espslpd', 'status ' + req.slot('STATUS'));
  res.say('Turning ' + req.slot('STATUS'));
console.log(req.data);
});
app.intent('SelectMenuFunction', {
  "slots": {"SELECT": "MENU_FUNCTION" },
  "utterances": ["{Mute|Menu|Exit|Info|Tools|Up|Down|Left|Right|Play|Pause|Stop}"]
}, function(req, res) {
  client.publish('espslpd', 'menu function: ' + req.slot('SELECT'));
  res.say(req.slot('SELECT'));
console.log(req.data);
});


module.exports = app;