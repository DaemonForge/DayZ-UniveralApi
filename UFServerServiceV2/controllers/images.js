
const express = require('express');
const router = express.Router();
const fs = require('fs');
const { promises: fsPromises } = fs;
const os = require('os');
const path = require('path');
const { exec, execSync } = require('child_process');
const https = require('https');
const http = require('http');
const { createLogger, NormalizeToGUID, ensureDirExsist } = require('../utils');
const logger = createLogger(global.logger, 'images');

// Import your Mongoose model for storing images.
//const SaveImage = require('../models/images').SaveImage;

// Import the Discord utility function.
const { GetDiscordObj } = require('../discord/dsUtils.js');
const crypto = require('crypto');

// Detect available image conversion tool at startup
let imageConversionTool = null;
let imageConversionAvailable = false;

function detectImageConversionTool() {
     if (process.platform === 'win32') {
          // Windows: Use texconv.exe
          let texconvPath;
          if (process.defaultApp) {
                texconvPath = path.join(global.rootPath || __dirname, 'bin', 'texconv.exe');
          } else {
                texconvPath = path.join(path.dirname(process.execPath), 'texconv.exe');
          }
          
          if (fs.existsSync(texconvPath)) {
                imageConversionTool = { type: 'texconv', path: texconvPath };
                imageConversionAvailable = true;
                logger.info(`Image conversion: Using texconv at ${texconvPath}`);
                return;
          }
     }
     
     // Linux/macOS: Check for ImageMagick (supports DDS with DXT5 compression)
     try {
          // Check if ImageMagick is installed and supports DDS
          const magickVersion = execSync('magick -version 2>/dev/null || convert -version 2>/dev/null', { 
                encoding: 'utf8',
                timeout: 5000,
                stdio: ['pipe', 'pipe', 'pipe']
          });
          
          if (magickVersion.includes('ImageMagick')) {
                // Check if DDS is supported
                const formats = execSync('magick identify -list format 2>/dev/null || identify -list format 2>/dev/null', {
                     encoding: 'utf8',
                     timeout: 5000,
                     stdio: ['pipe', 'pipe', 'pipe']
                });
                
                if (formats.includes('DDS')) {
                     // Determine if using ImageMagick 7 (magick command) or 6 (convert command)
                     const useMagick7 = magickVersion.includes('Version: ImageMagick 7');
                     imageConversionTool = { 
                          type: 'imagemagick', 
                          command: useMagick7 ? 'magick' : 'convert',
                          version: useMagick7 ? 7 : 6
                     };
                     imageConversionAvailable = true;
                     logger.info(`Image conversion: Using ImageMagick ${useMagick7 ? '7' : '6'} for DDS conversion`);
                     return;
                }
          }
     } catch (e) {
          // ImageMagick not available or error checking
          logger.debug('ImageMagick not detected or DDS not supported');
     }
     
     // Fallback: Check for texconv on Linux (wine or native build)
     if (process.platform !== 'win32') {
          const texconvLinuxPath = path.join(path.dirname(process.execPath), 'bin', 'texconv');
          if (fs.existsSync(texconvLinuxPath)) {
                imageConversionTool = { type: 'texconv-linux', path: texconvLinuxPath };
                imageConversionAvailable = true;
                logger.info(`Image conversion: Using Linux texconv at ${texconvLinuxPath}`);
                return;
          }
     }
     
     logger.warn('Image conversion: No supported tool found (texconv or ImageMagick with DDS support)');
     logger.warn('On Linux, install ImageMagick: apt install imagemagick or dnf install ImageMagick');
}

// Detect tool on module load
detectImageConversionTool();

/*
    Helper: downloadFile
*/
function downloadFile(fileUrl, destination) {
     const client = fileUrl.startsWith('https') ? https : http;
     return new Promise((resolve, reject) => {
          client.get(fileUrl, (response) => {
                if (response.statusCode !== 200) {
                     logger.warn(`downloadFile: Failed to download file. HTTP Status Code: ${response.statusCode}`);
                     return reject(new Error(`Failed to download file. HTTP Status Code: ${response.statusCode}`));
                }
                const file = fs.createWriteStream(destination);
                response.pipe(file);
                file.on('finish', () => {
                     logger.debug(`downloadFile: File downloaded to ${destination}`);
                     file.close(resolve);
                });
                file.on('error', (err) => {
                     fs.unlink(destination, () => {}); // Cleanup on error.
                     logger.error(`downloadFile: Error writing file: ${err.message}`);
                     reject(err);
                });
          }).on('error', (err) => {
                logger.error(`downloadFile: Request error: ${err.message}`);
                reject(err);
          });
     });
}

/*
    Helper: execCommand
*/
function execCommand(command) {
     logger.debug(`execCommand: Executing command: ${command}`);
     return new Promise((resolve, reject) => {
          exec(command, (error, stdout, stderr) => {
                if (error) {
                     logger.error(`execCommand: Command failed: ${command}\nError: ${error.message}`);
                     return reject(new Error(`Command failed: ${command}\nError: ${error.message}`));
                }
                logger.debug(`execCommand: stdout: ${stdout}, stderr: ${stderr}`);
                resolve({ stdout, stderr });
          });
     });
}

/*
    Helper: convertPngToDdsBase64
    Supports multiple backends:
    - Windows: texconv.exe (DirectX)
    - Linux/macOS: ImageMagick (magick/convert with DDS support)
*/
async function convertPngToDdsBase64(imageUrl) {
     if (!imageConversionAvailable || !imageConversionTool) {
          throw new Error('Image conversion to DDS is not available. On Windows, ensure texconv.exe is present. On Linux, install ImageMagick with DDS support.');
     }
     
     logger.info(`convertPngToDdsBase64: Converting image from ${imageUrl}`);
     const randomNum = Math.floor(Math.random() * 90000) + 10000;
     const urlHash = crypto.createHash('sha256').update(`${imageUrl}+${randomNum}`).digest('hex');
     const tmpDir = ensureDirExsist('temp');
    
     const inputFilePath = path.join(tmpDir, `${urlHash}.png`);
     const ddsOutputFile = path.join(tmpDir, `${urlHash}.dds`);
     
     // Cleanup any existing files
     if (fs.existsSync(inputFilePath)) {
          fs.unlinkSync(inputFilePath);
     }
     if (fs.existsSync(ddsOutputFile)) {    
          fs.unlinkSync(ddsOutputFile);
     }
     
     // Download the PNG
     await downloadFile(imageUrl, inputFilePath);

     // Build and run the conversion command based on available tool
     let conversionCommand;
     
     switch (imageConversionTool.type) {
          case 'texconv':
                // Windows texconv.exe
                conversionCommand = `"${imageConversionTool.path}" -f DXT5 -o "${tmpDir}" "${inputFilePath}"`;
                break;
                
          case 'imagemagick':
                // ImageMagick (Linux/macOS/Windows)
                // Use DXT5 compression for DDS output
                conversionCommand = `${imageConversionTool.command} "${inputFilePath}" -define dds:compression=dxt5 "${ddsOutputFile}"`;
                break;
                
          case 'texconv-linux':
                // Linux native texconv (if available)
                conversionCommand = `"${imageConversionTool.path}" -f DXT5 -o "${tmpDir}" "${inputFilePath}"`;
                break;
                
          default:
                throw new Error('Unknown image conversion tool type');
     }
     
     logger.info(`convertPngToDdsBase64: Running conversion command`);
     await execCommand(conversionCommand);

     // Read the DDS file and return its Base64 encoding
     const ddsBuffer = await fsPromises.readFile(ddsOutputFile);
     const base64DDS = ddsBuffer.toString('base64');
     logger.info(`convertPngToDdsBase64: Conversion successful`);

     // Cleanup temporary files
     try {
          fs.unlinkSync(inputFilePath);
          fs.unlinkSync(ddsOutputFile);
     } catch(err) {
          logger.error(`convertPngToDdsBase64: Error cleaning up temporary files: ${err.message}`);
     }
     
     return base64DDS;
}

/*
    Endpoint: POST /Generate
*/
router.post('/Generate', async (req, res) => {
     const imageUrl = req.body.ImageURL;
     logger.info(`/Generate: Received request for ${imageUrl}`);
     if (!imageUrl) {
          logger.warn(`/Generate: Missing ImageURL`);
          return res.status(400).json({ error: "ImageURL is required" });
     }
     try {
          // Create a new image record with pending status.
          const newImage = new SaveImage({ imageUrl, base64: null, status: "pending" });
          await newImage.save();
          logger.debug(`/Generate: New image record created with id ${newImage._id}`);

          // Return the ImageId immediately.
          res.json({ ImageId: newImage._id });

          // Start the conversion asynchronously.
          convertPngToDdsBase64(imageUrl)
                .then(async (base64DDS) => {
                     newImage.base64 = base64DDS;
                     newImage.status = "ready";
                     await newImage.save();
                     logger.info(`/Generate: Image ${newImage._id} conversion completed successfully`);
                })
                .catch(async (err) => {
                     newImage.status = "error";
                     newImage.error = err.message;
                     await newImage.save();
                     logger.error(`/Generate: Image ${newImage._id} conversion failed: ${err.message}`);
                });
     } catch (err) {
          logger.error(`/Generate: Error: ${err.message}`,{err});
          return res.status(500).json({ error: err.message });
     }
});

/*
    Endpoint: POST /Download/:imageId
*/
router.post('/Download/:imageId', async (req, res) => {
     const imageId = req.params.imageId;
     logger.debug(`/Download/${imageId}: Request received`);
     try {
          const imageRecord = await SaveImage.findById(imageId);
          if (!imageRecord) {
                logger.warn(`/Download/${imageId}: Image not found`);
                return res.status(404).json({ error: "Image not found" });
          }
          if (!imageRecord.base64) {
                logger.debug(`/Download/${imageId}: Image still processing - status: ${imageRecord.status}`);
                return res.status(202).send("Image is still processing: " + imageRecord.status);
          }
          logger.debug(`/Download/${imageId}: Returning Base64 data`);
          res.set('Content-Type', 'text/plain');
          return res.send(imageRecord.base64);
     } catch (err) {
          logger.error(`/Download/${imageId}: Error: ${err.message}`);
          return res.status(500).json({ error: err.message });
     }
});

/*
    Endpoint: POST /Download
*/
router.post('/Download', async (req, res) => {
     const imageUrl = req.body.ImageURL;
     logger.debug(`/Download: Request received for ${imageUrl}`);
     if (!imageUrl) {
          logger.warn(`/Download: Missing ImageURL`);
          return res.status(400).json({ error: "ImageURL is required" });
     }
     try {
          const base64DDS = await convertPngToDdsBase64(imageUrl);
          logger.debug(`/Download: Conversion completed for ${imageUrl}`);
          res.set('Content-Type', 'text/plain');
          return res.send(base64DDS);
     } catch (err) {
          logger.error(`/Download: Error converting image for ${imageUrl}: ${err.message}`);
          return res.status(500).json({ error: err.message });
     }
});

/*
    Endpoint: POST /Discord/:GUID
*/
router.post('/Discord/:GUID', async (req, res) => {
     if (req.params.GUID === "own") {
          req.params.GUID = req.GUID;
     }
     const GUID = NormalizeToGUID(req.params.GUID);
     logger.debug(`/Discord/${GUID}: Request received`);
     try {
          const discordObj = await GetDiscordObj(GUID);
          if (!discordObj || !discordObj.avatar) {
                logger.warn(`/Discord/${GUID}: Discord avatar not found`);
                return res.status(404).send("NotFound");
          }
          const base64DDS = await convertPngToDdsBase64(discordObj.avatar);
          logger.debug(`/Discord/${GUID}: Conversion successful`);
          return res.send(base64DDS);
     } catch (err) {
          logger.error(`/Discord/${GUID}: Error: ${err.message}`, {err});
          return res.status(500).send("Error");
     }
});

module.exports = router;
