// Required modules.
const express = require('express');
const router = express.Router();
const crypto = require('crypto');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { spawn } = require('child_process');
const OpenAI = require('openai').default;
const AudioModel = require('../models/tts');
const { createLogger, ensureDirExsist } = require('../utils');
const { requirePlayerOrServerAuth } = require('../auth/utils');
const logger = createLogger(global.logger, 'TTS');

// Conditionally load Electron app module (only available in Electron environment)
let electronApp = null;
try {
  if (global.isElectron) {
    electronApp = require('electron').app;
  }
} catch (e) {
  // Not running in Electron environment
}

// ================================================================
// Helper Function: getFfmpegPath
//
// Returns the appropriate FFmpeg binary path based on how the application is running.
//   1. Packaged Electron app (but not using pkg): binary is expected to be unpacked 
//      next to the Electron executable.
//   2. pkg bundle: For Linux, first checks system PATH, then bundled binary.
//      For Windows, uses the ffmpeg-static module bundled asset.
//   3. In development, it simply returns the path from the ffmpeg-static module.
function getFfmpegPath() {
  // Determine the correct binary filename based on the platform.
  // Windows requires the ".exe" extension.
  const binaryName = process.platform === "win32" ? "ffmpeg.exe" : "ffmpeg";

  // 1. Running as a packaged Electron app (non-pkg).
  if (electronApp && electronApp.isPackaged && !process.pkg) {
    // The binary is expected to be unpacked next to the Electron executable.
    return path.join(path.dirname(process.execPath), binaryName);
  }
  // 2. Running in a pkg bundle.
  else if (process.pkg) {
    // For Linux, first check if ffmpeg is available in system PATH
    if (process.platform === "linux" || process.platform === "darwin") {
      try {
        const { execSync } = require('child_process');
        const systemFfmpeg = execSync('which ffmpeg 2>/dev/null', { encoding: 'utf8' }).trim();
        if (systemFfmpeg && fs.existsSync(systemFfmpeg)) {
          logger.info(`Using system FFmpeg at: ${systemFfmpeg}`);
          return systemFfmpeg;
        }
      } catch (e) {
        // System ffmpeg not found, continue to bundled binary
        logger.debug('System FFmpeg not found in PATH, checking bundled binary');
      }
      
      // Check for bundled binary in bin folder
      const bundledPath = path.join(path.dirname(process.execPath), 'bin', binaryName);
      if (fs.existsSync(bundledPath)) {
        // Copy to temp and make executable if needed
        const tempDir = os.tmpdir();
        const targetPath = path.join(tempDir, `uf-${binaryName}`);
        if (!fs.existsSync(targetPath)) {
          fs.copyFileSync(bundledPath, targetPath);
          try {
            fs.chmodSync(targetPath, 0o755);
          } catch (e) {
            logger.warn('Failed to set ffmpeg executable permissions');
          }
        }
        return targetPath;
      }
      
      // Fallback: return system path and let it fail gracefully if not found
      logger.warn('FFmpeg not found. Install with: apt install ffmpeg (Debian/Ubuntu) or dnf install ffmpeg (Fedora)');
      return 'ffmpeg';
    } else {
      // For Windows, use the binary from ffmpeg-static.
      const sourcePath = path.join(path.dirname(process.execPath), 'node_modules', 'ffmpeg-static', binaryName);
      const tempDir = os.tmpdir();
      const targetPath = path.join(tempDir, binaryName);

      // If the binary hasn't been extracted yet, copy it from the source path.
      if (!fs.existsSync(targetPath) && fs.existsSync(sourcePath)) {
        fs.copyFileSync(sourcePath, targetPath);
      }
      return fs.existsSync(targetPath) ? targetPath : sourcePath;
    }
  }
  // 3. In development mode, use the binary provided by the ffmpeg-static module.
  else {
    try {
      return require('ffmpeg-static');
    } catch (e) {
      // Fallback to system ffmpeg
      return 'ffmpeg';
    }
  }
}

// Obtain the ffmpeg binary path using our helper.
let ffmpegPath;
try {
  ffmpegPath = getFfmpegPath();
} catch (e) {
  logger.error('Failed to determine FFmpeg path:', e.message);
  ffmpegPath = 'ffmpeg'; // Fallback to system PATH
}

// ----------------------------------------------------------------
// Log status of ffmpeg binary
if (ffmpegPath === 'ffmpeg') {
  logger.info('Using system FFmpeg from PATH');
} else if (!fs.existsSync(ffmpegPath)) {
  logger.error(`FFmpeg binary not found at path: ${ffmpegPath}`);
  logger.error('TTS features will not work. On Linux, install with: apt install ffmpeg');
} else {
  logger.debug(`FFmpeg binary found at: ${ffmpegPath}`);
}

// Ensure the audio cache directory exists.
const audioCachePath = path.join(global.SAVEPATH, 'audioCache');
if (!fs.existsSync(audioCachePath)) {
  fs.mkdirSync(audioCachePath, { recursive: true });
}

/**
 * Helper: getAudioSampleRate
 * Uses FFmpeg to probe the input file and extract its sample rate.
 * Returns a Promise that resolves to the sample rate as a string.
 */
function getAudioSampleRate(inputPath) {
  return new Promise((resolve, reject) => {
    const ffmpegProbe = spawn(ffmpegPath, ['-hide_banner', '-i', inputPath]);
    let errorOutput = "";
    ffmpegProbe.stderr.on('data', (data) => {
      errorOutput += data.toString();
    });
    ffmpegProbe.on('close', () => {
      // Try to match a line like: "Stream #0:1: Audio: mp3, 44100 Hz, mono, s16p, 64 kb/s"
      const match = errorOutput.match(/Audio:.*? (\d+)\s*Hz/);
      if (match && match[1]) {
        resolve(match[1]);
      } else {
        resolve("8000"); // fallback if sample rate cannot be determined
      }
    });
    ffmpegProbe.on('error', (err) => {
      reject(err);
    });
  });
}

/**
 * Helper: standardizeAudioFile
 * Re-encodes the underlying TTS-generated MP3 file into a standardized WAV file.
 * This forces a sample rate of 44100 Hz, stereo, and 16-bit PCM output.
 *
 * Using a WAV file is more forgiving and avoids misdetection issues that sometimes occur with MP3.
 *
 * The outputPath should have a .wav extension.
 */
function standardizeAudioFile(inputPath, outputPath, targetSampleRate = "44100") {
  return new Promise((resolve, reject) => {
    // Here we use PCM S16LE encoding in a WAV container.
    const args = [
      '-y',                        // Overwrite output.
      '-fflags', '+genpts',         // Generate PTS if missing.
      '-err_detect', 'ignore_err',  // Ignore minor errors.
      '-analyzeduration', '100M',   // Increase analysis duration.
      '-probesize', '100M',         // Increase probe size.
      '-i', inputPath,             // Input file.
      '-ar', targetSampleRate,     // Force sample rate.
      '-ac', '2',                  // Force stereo.
      '-c:a', 'pcm_s16le',         // Use 16-bit PCM encoding.
      outputPath                   // Output file (WAV container).
    ];
    
    const proc = spawn(ffmpegPath, args);
    
    proc.stderr.on('data', (data) => {
      logger.debug(`Standardize audio output: ${data.toString().trim()}`, data.toString().trim());
    });
    
    proc.on('close', (code) => {
      if (code === 0) {
        resolve();
      } else {
        reject(new Error(`Audio standardization failed with code ${code}`));
      }
    });
    
    proc.on('error', (err) => {
      reject(err);
    });
  });
}

/**
 * Normalize the input visual mode.
 * Mapping:
 *   "waveform" or "line"   -> "line"
 *   "none"                 -> "none"
 * Default is "line".
 */
function normalizeVisualMode(input) {
  if (!input) return "line";
  const v = input.toLowerCase();
  if (v === "waveform" || v === "line") return "line";
  if (v === "none") return "none";
  return "line";
}

/*
  POST /Generate/:VoiceID
  Expected JSON body:
    {
      "Message": "Text to convert to speech",
      "StaticNoise": 0.5,
      "Instructions": "Speak in a cheerful and positive tone.",
      "Visual": "waveform"
    }
  This endpoint creates a new audio job (via AudioModel.createJob) and starts asynchronous processing.
*/
router.post('/Generate/:VoiceID', requirePlayerOrServerAuth, async (req, res) => {
  try {
    const { VoiceID } = req.params;

    const message = req.body.Message ? req.body.Message.trim() :
      `This is a generated audio with voice ${VoiceID}.`;
    const instructions = req.body.Instructions ? req.body.Instructions.trim() : "";

    let staticLevel = parseFloat(req.body.StaticNoise);
    if (isNaN(staticLevel) || staticLevel < 0 || staticLevel > 1) {
      staticLevel = 0.0;
    }

    const visualMode = normalizeVisualMode(req.body.Visual);

    // Create a new job in MongoDB.
    const jobId = await AudioModel.createJob(VoiceID, message, instructions, staticLevel, visualMode);

    // Start asynchronous audio processing.
    processAudioClip(jobId, VoiceID, message, instructions, staticLevel, visualMode)
      .catch(err => logger.error(`Error processing audio clip ${jobId}: ${err.message}`, { error: err.message }));

    res.json({ TTSId: jobId });
  } catch (error) {
    logger.error("Error in /Generate route", { error: error.message });
    res.status(500).json({ error: error.message });
  }
});

/*
  GET /Status/:TTSId
  Returns the current job status.
*/
router.get('/Status/:TTSId', requirePlayerOrServerAuth, async (req, res) => {
  try {
    const { TTSId } = req.params;
    const job = await AudioModel.getJob(TTSId);
    if (!job) return res.status(200).json({ Status: "NotFound" });
    res.json({ Status: job.status });
  } catch (error) {
    res.status(200).json({ Status: "Error" });
  }
});

/*
  POST /Download/:TTSId
  Returns the Base64 encoded MP4 file if the job succeeded.
*/
router.post('/Download/:TTSId', requirePlayerOrServerAuth, async (req, res) => {
  try {
    logger.info("TTS Download request", { params: req.params });
    const { TTSId } = req.params;
    const job = await AudioModel.getJob(TTSId);
    if (!job ) {
      logger.warn("TTS Download request: Not found", { params: req.params });
      return res.send("NotFound");
    }
    if (job.status !== 'Success' ) return res.send(job.status);
    res.send(job.file);
  } catch (error) {
    logger.info(`TTS Download request Error ${error.message}`, {error, params: req.params });
    res.send("Error");
  }
});

/*
  processAudioClip:
    - Computes a hash from (message + instructions) for caching.
    - If the MP3 is not cached, calls the OpenAI TTS API.
    - After obtaining the MP3, we standardize it by converting it into a WAV file
      with a fixed sample rate of 44100 Hz and stereo channels.
    - The standardized WAV file is then passed into convertToMp4.
    - Finally, the resulting MP4 is Base64-encoded and stored in MongoDB.
*/
async function processAudioClip(jobId, voiceId, message, instructions, staticLevel, visualMode) {
  try {
    const hash = crypto.createHash('sha512').update(message + instructions).digest('hex');
    const mp3Filename = hash + ".mp3";
    const mp3FilePath = path.join(audioCachePath, mp3Filename);
    const standardizedAudioFilename = "std_" + hash + ".wav";
    const standardizedAudioPath = path.join(audioCachePath, standardizedAudioFilename);

    if (!fs.existsSync(standardizedAudioPath)) {
      logger.info("Audio File not found in cache. Calling OpenAI TTS API.", { hash });
      const openai = new OpenAI({ apiKey: global.config.OpenAIApi.ApiKey });
      const mp3Response = await openai.audio.speech.create({
        model: "gpt-4o-mini-tts",
        voice: voiceId,
        input: message,
        instructions: instructions,
      });
      const arrayBuffer = await mp3Response.arrayBuffer();
      const buffer = Buffer.from(arrayBuffer);
      fs.writeFileSync(mp3FilePath, buffer);
      logger.info(`Standardizing audio file ${hash}`, { original: mp3FilePath, standardized: standardizedAudioPath });
      await standardizeAudioFile(mp3FilePath, standardizedAudioPath, "44100");
      fs.unlinkSync(mp3FilePath);
    } else {
      logger.info("Using cached File.", { hash });
    }
    
    // Convert the standardized WAV to an MP4 video.
    const tempDir = ensureDirExsist('temp');
    const mp4FilePath = path.join(tempDir, jobId + ".mp4");
    await convertToMp4(standardizedAudioPath, mp4FilePath, staticLevel, visualMode);
    
    const fileBuffer = fs.readFileSync(mp4FilePath);
    const fileBase64 = fileBuffer.toString('base64');
    
    await AudioModel.updateJob(jobId, {
      status: 'Success',
      file: fileBase64,
      updatedAt: new Date()
    });
    
    // cleanup the temporary MP4 file.
    fs.unlinkSync(mp4FilePath);
  } catch (error) {
    logger.error(`Error processing audio clip ${error.message}`, { error: error.message });
    await AudioModel.updateJob(jobId, {
      status: 'Error',
      error: error.message,
      updatedAt: new Date()
    });
  }
}

/*
  convertToMp4:
    Converts an audio file (now a standardized WAV file) into an MP4 video with a visualization overlay.
    Supports:
      - "line": waveform visualization.
      - "none": black background.
    In every branch the audio is passed through a filter chain that splits and resamples it using the detected sample rate.
*/
async function convertToMp4(inputPath, outputPath, staticLevel, visualMode) {
  // Probe the standardized file for its sample rate.
  let sampleRate;
  try {
    sampleRate = await getAudioSampleRate(inputPath);
    logger.info("TTS audio sample rate detected", { sampleRate });
  } catch (err) {
    logger.warn("Could not retrieve sample rate, defaulting to 8000 Hz", { error: err.message });
    sampleRate = "8000";
  }
  
  // Base encoding arguments (for video output; audio is processed in the filter chain).
  const baseArgs = [
    '-c:v', 'libx264',
    '-profile:v', 'baseline',
    '-preset', 'veryslow',
    '-crf', '40',
    '-pix_fmt', 'yuv420p',
    '-c:a', 'aac',
    '-b:a', '16k',
    '-r', '8'  // output video fps
  ];
  
  let ffmpegArgs = [];
  
  if (visualMode === "line") {
    if (staticLevel <= 0) {
      // No static noise: split the audio so one branch is used for showwaves and the other is resampled.
      const filterComplex = `[0:a]asplit=2[audio][waveIn]; ` +
                              `[waveIn]showwaves=s=48x48:mode=line:colors=rainbow[v]; ` +
                              `[audio]aresample=${sampleRate}[audioOut]`;
      ffmpegArgs = [
        '-y',
        '-i', inputPath,
        '-filter_complex', filterComplex,
        '-map', '[v]',
        '-map', '[audioOut]'
      ].concat(baseArgs, [outputPath]);
    } else {
      // With static noise: mix dynamic noise, reset timestamps, split, and resample.
      const randomSeed = Math.floor(Math.random() * 100000);
      const noiseInput = `anoisesrc=seed=${randomSeed}:c=white`;
      const randomPeriod = (Math.random() * 3 + 3).toFixed(2);
      const randomOnTime = (Math.random() * 0.3 + 0.3).toFixed(2);
      const volumeExpr = `${staticLevel}*(0.18+if(lt(mod(t,${randomPeriod}),${randomOnTime}),(0.4+0.1*sin(20*t)),(0.05+0.05*sin(5*t))))`;
      
      const filterComplex = "[1:a]volume='" + volumeExpr + "':eval=frame[static]; " +
                            "[0:a][static]amix=inputs=2:duration=first:weights='1 0.8'[mix]; " +
                            "[mix]asetpts=PTS-STARTPTS,asplit=2[audio][waveIn]; " +
                            "[waveIn]showwaves=s=48x48:mode=line:colors=rainbow[v]; " +
                            "[audio]aresample=" + sampleRate + "[audioOut]";
    
      ffmpegArgs = [
        '-y',
        '-i', inputPath,
        '-f', 'lavfi',
        '-i', noiseInput,
        '-filter_complex', filterComplex,
        '-map', '[v]',
        '-map', '[audioOut]'
      ].concat(baseArgs, [outputPath]);
    }
  } else {
    // visualMode "none": black background.
    if (staticLevel <= 0) {
      const filterComplex = `[0:a]aresample=${sampleRate}[audioOut]`;
      ffmpegArgs = [
        '-y',
        '-i', inputPath,
        '-f', 'lavfi',
        '-i', 'color=c=black:s=48x48',
        '-filter_complex', filterComplex,
        '-map', '1:v',
        '-map', '[audioOut]'
      ].concat(baseArgs, ['-shortest', outputPath]);
    } else {
      const randomSeed = Math.floor(Math.random() * 100000);
      const noiseInput = `anoisesrc=seed=${randomSeed}:c=white`;
      const randomPeriod = (Math.random() * 3 + 3).toFixed(2);
      const randomOnTime = (Math.random() * 0.3 + 0.3).toFixed(2);
      const volumeExpr = `${staticLevel}*(0.18+if(lt(mod(t,${randomPeriod}),${randomOnTime}),(0.4+0.1*sin(20*t)),(0.05+0.05*sin(5*t))))`;
      
      const filterComplex = "[1:a]volume='" + volumeExpr + "':eval=frame[static]; " +
                              "[0:a][static]amix=inputs=2:duration=first:weights='1 0.8'[mix]; " +
                              "[mix]asetpts=PTS-STARTPTS,aresample=" + sampleRate + "[audioOut]";
      
      ffmpegArgs = [
        '-y',
        '-i', inputPath,
        '-f', 'lavfi',
        '-i', noiseInput,
        '-f', 'lavfi',
        '-i', 'color=c=black:s=48x48',
        '-filter_complex', filterComplex,
        '-map', '2:v',
        '-map', '[audioOut]'
      ].concat(baseArgs, ['-shortest', outputPath]);
    }
  }
  
  // Spawn FFmpeg with the constructed arguments.
  return new Promise((resolve, reject) => {
    const ffmpegProc = spawn(ffmpegPath, ffmpegArgs);
    
    ffmpegProc.stderr.on('data', (data) => {
      logger.debug(`FFmpeg processing:${data.toString().trim()}`, {
        data: data.toString().trim(),
        inputPath,
        outputPath
      });
    });
    
    ffmpegProc.on('close', (code) => {
      if (code === 0) {
        resolve();
      } else {
        const errorMessage = `FFmpeg exited with code ${code}`;
        logger.error(errorMessage, { inputPath, outputPath });
        reject(new Error(errorMessage));
      }
    });
    
    ffmpegProc.on('error', (err) => {
      logger.error('FFmpeg encountered an error', { error: err.message, inputPath, outputPath });
      reject(err);
    });
  });
}

module.exports = router;
