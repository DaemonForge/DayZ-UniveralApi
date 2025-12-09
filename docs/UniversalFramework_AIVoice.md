# Universal Framework - AI Voice (TTS)

## Overview

Text-to-Speech using OpenAI. Generates audio files that can be played in-game.

## UTTSMessage Object

```enforce
class UTTSMessage extends UFObject_Base {
    string Message;
    float StaticNoise;
    string Instructions;  // Personality/voice instruction
    string Visual;        // UTTSVisual.LINE or NONE
    
    void UTTSMessage(string msg, 
        string instruc = UTTSPersonality.RAGED_SURVIVOR, 
        float staticNoise = 0.0, 
        string visual = UTTSVisual.LINE);
}
```

## API Endpoint Methods

Access via `U().Api()`:

```enforce
// Generate TTS audio (returns audio ID in callback)
int TTSGenerate(string voiceID, UTTSMessage msg, Class cbInstance, string cbFunction);

// Check generation status
int TTSStatus(string ttsId, Class cbInstance, string cbFunction);

// Download audio file (client-only, saves to $saves:{ttsId}.mp4)
int TTSDownload(string ttsId);
int TTSDownload(string ttsId, Class cbInstance, string cbFunction);

// Download and play (client-only, auto-caches)
int TTSPlay(string ttsId);
```

## Usage Example

```enforce
// Generate TTS
UTTSMessage msg = new UTTSMessage("Hello survivor!");
U().Api().TTSGenerate(UTTSVoice.ALLOY, msg, this, "OnTTSGenerated");

void OnTTSGenerated(int cid, int status, string audioId, string result) {
    if (status == UF_SUCCESS) {
        // Audio ID ready - poll status or download
        U().Api().TTSStatus(audioId, this, "OnTTSStatus");
    }
}

void OnTTSStatus(int cid, int status, string oid, string statusText) {
    if (status == UF_SUCCESS) {
        // Ready to download
        U().Api().TTSDownload(oid, this, "OnTTSDownloaded");
    }
}

void OnTTSDownloaded(int cid, int status, string oid, string result) {
    if (status == UF_SUCCESS) {
        string audioPath = "$saves:" + oid + ".mp4";
        // Audio file ready
    }
}
```

## Callback Classes

### UFDownloadTTS

Downloads TTS audio and saves to file.

```enforce
class UFDownloadTTS : UFRestCallBackBase {
    string m_oid;  // Audio ID for filename
    
    void UFDownloadTTS(string oid);
    
    // On success: saves to $saves:{oid}.mp4
}
```

### Usage

```enforce
// The service returns base64 audio data
// UFDownloadTTS automatically saves to:  $saves:{audioId}.mp4
autoptr UFDownloadTTS cb = new UFDownloadTTS("my_audio_id");
// After API call, file saved to: $saves:my_audio_id.mp4
```

### UDLTTSCallback

Notifies when download completes.

```enforce
void OnTTSReady(int cid, int status, string oid, string result) {
    if (status == UF_SUCCESS) {
        string audioPath = "$saves:" + oid + ".mp4";
        // Audio file ready to play
    }
}
```

### UTTSStatusCallback

Checks if TTS generation is complete.

```enforce
void OnTTSStatus(int cid, int status, string oid, string statusText) {
    // status: UF_SUCCESS (ready), UF_NOTFOUND, UF_EMPTY, UF_ERROR
    // statusText: "Success", "NotFound", "Empty", "Error"
}
```

## Available Voices (UTTSVoice)

```enforce
UTTSVoice.ALLOY    // Neutral, balanced
UTTSVoice.ASH
UTTSVoice.BALLAD
UTTSVoice.CORAL
UTTSVoice.ECHO     // Male, warm
UTTSVoice.FABLE    // Expressive
UTTSVoice.ONYX     // Deep, authoritative
UTTSVoice.NOVA     // Female, friendly
UTTSVoice.SAGE
UTTSVoice.SHIMMER  // Clear, gentle
UTTSVoice.VERSE
```

## Notes

- Audio saved as MP4 format
- Files stored in `$saves:` directory
- TTS generation is async - poll status or use callbacks
- Cache generated audio to reduce API calls
