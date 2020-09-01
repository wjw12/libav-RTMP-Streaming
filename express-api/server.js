const express = require('express');
const bodyParser = require('body-parser');
const {exec, spawn} = require('child_process');
const { uniqueNamesGenerator, adjectives, colors, animals } = require('unique-names-generator');

const app = express();
const router = express.Router();
app.use(bodyParser.json());
app.use('/stream', router);

const streams = {};
const containers = {}; // map stream name to unique container name

const serverUrl = process.env.HOST_STREAMING || "stream.jiewen.wang";

router.get('/', (req, res, next) => {
  if (Object.keys(req.params).length > 0) {next(); return;}
  res.send(Object.keys(streams));
});

app.post('/stream', (req, res) => {
  const name = req.body.name;
  const url = req.body.url;
  if (!name || !url) {res.status(400).send({"success": false}); return;}
  if (streams[name]) {res.status(200).send({"success": true}); return;}
  const containerName = uniqueNamesGenerator({ dictionaries: [adjectives, colors, animals] });
  const docker = spawn('docker', ('run --name '+containerName+' --network host --rm -e SRC='+url+' -e RTMP=rtmp://127.0.0.1/live/'+name+' -e RTSP=rtsp://127.0.0.1/'+name+' streamer:latest').split(' '));
  docker.stdout.on('data', (data) => { console.log(`stdout: ${data}`);});
  docker.stderr.on('data', (data) => {console.log(`stderr: ${data}`);});
  docker.on('close', (code) => {console.log(`child process exited with code ${code}`);});
  
  containers[name] = containerName;
  streams[name] = {
    "name": name,
    "url": url,
    "rtmp": "rtmp://" + serverUrl + ":1935/live/" + name,
    "rtsp": "rtsp://" + serverUrl + ":8554/" + name,
  }
  res.status(200).send({"success": true});
});

router.get('/:name', (req, res) => {
  const name = req.params.name;
  if (!name || !streams[name]) {res.status(200).send({"success": false}); return;}
  res.status(200).send(streams[name]);
});

router.delete('/:name', (req, res) => {
  const name = req.params.name;
  if (!name || !streams[name]) {res.status(200).send({"success": false}); return;}

  const containerName = containers[name];
  const docker = spawn('docker', ['stop', containerName]);
  docker.on('close', (code) => {console.log(`docker ${containerName} has stopped`);});
  delete streams[name];
  delete containers[name];
  
  res.status(200).send({"success": true});
});

app.listen(80, () => console.log('API server running'));
