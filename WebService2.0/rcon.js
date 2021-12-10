const {
    Router
} = require('express');
const log = require("./log");
const {
    isArray,
    GenerateLimiter,
    IncermentAPICount,
    byteSize
} = require('./utils');
const {UDP_RCON} = require('@hidev/udp-rcon');
const router = Router();

router.post('/', (req, res) => {
});
const server_ip = "198.50.221.57";
const rcon_port = "2305";
const rcon_password = "3c831021bf1feb29d13b3a76e8f8eb94";
const rcon_success = (msg) => {
    console.log("Message received:", msg);
};

const rcon_error = (err) => {
    console.log(`Error: \n${err.stack}`);
};

TestRcon();

async function TestRcon(){
  let my_rcon = new UDP_RCON(server_ip, rcon_port, rcon_password);
  
  // Commands
  let rcon_instance = my_rcon.send("say Hello!", rcon_success, rcon_error);
  console.log(rcon_instance)
}

module.exports = router;