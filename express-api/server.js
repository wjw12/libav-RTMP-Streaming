const express = require('express');
const bodyParser = require('body-parser');
const {exec} = require('child_process');

const app = express();
app.use(bodyParser.json());

const streams = {};

const serverUrl = process.env.HOST_STREAMING || "stream.jiewen.wang";

app.get('/stream', (req, res) => {
  res.send(Object.keys(streams));
});

app.post('/stream', (req, res) => {
  const name = req.body.name;
  const url = req.body.url;
  if (!name || !url) {res.status(400).send({"success": false}); return;}
  if (streams[name]) {res.status(200).send(streams[name]); return;}

  // launch process
  //exec('./libav-RTMP-Streaming/build/streamer', {env: {SRC: url, DST: 'rtmp://127.0.0.1/live/' + name }},
  exec('cd /files && make build && docker run -e SRC=' + url + ' -e DST=rtmp://127.0.0.1/live/' + name + ' leandromoreira/ffmpeg-devel /files/build/streamer',
  (err, stdout, stderr) => {
    if (err) {console.log(`error: ${err.message}`); res.status(400).send({"success": false, "msg": err.message}); return;}
    if (stderr) {console.log(`stderr: ${stderr}`); res.status(400).send({"success": false, "msg": stderr}); return;}
    console.log(`stdout: ${stdout}`);
  });
  
  streams[name] = {
    "name": name,
    "url": url,
    "rtmp": "rtmp://" + serverUrl + ":1935/live/" + name,
    "rtsp": "rtsp://" + serverUrl + ":8554/" + name,
  }
  res.status(200).send({"success": true});
});

app.get('/stream', (req, res) => {
  const name = req.query.name;
  if (!name || !streams[name]) {res.status(200).send({"success": false}); return;}
  res.status(200).send(streams[name]);
});

app.listen(80, () => console.log('API server running'));
