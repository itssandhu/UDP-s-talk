const express = require('express');
const bodyParser = require('body-parser');
const { exec } = require('child_process');

const app = express();
const port = 3000;

app.use(bodyParser.urlencoded({ extended: true }));
app.use(bodyParser.json());
app.use(express.static('public'));

app.post('/start_talking', (req, res) => {
    const localPort = req.body.local_port;
    const remoteIP = req.body.remote_ip;
    const remotePort = req.body.remote_port;

    // Call your C program here using the provided parameters
    const command = `cd ../c_code && make && ./s-talk ${localPort} ${remoteIP} ${remotePort}`;

    exec(command, (error, stdout, stderr) => {
        if (error) {
            console.error(`Error: ${error.message}`);
            return res.status(500).send('Internal Server Error');
        }

        console.log(`stdout: ${stdout}`);
        console.error(`stderr: ${stderr}`);

        res.send('UDP Talking Started!');
    });
});

app.listen(port, () => {
    console.log(`Server running at http://localhost:${port}`);
});
