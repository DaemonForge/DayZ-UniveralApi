window.api.onLogMessage((event, message) => {
  try {
    const logObj = JSON.parse(message);
    const { level, message: msg, timestamp } = logObj;
    
    const logEntry = document.createElement('div');
    logEntry.classList.add('log-entry', `log-${level}`);
    
    // Create HTML with a small colored dot and inline text
    logEntry.innerHTML = `
      <span class="log-color"></span>
      <span class="timestamp">${timestamp}</span>
      <span class="level">${level.toUpperCase()}</span>
      <span class="msg">${msg}</span>
    `;
    
    const logContainer = document.getElementById('logs');
    logContainer.appendChild(logEntry);
    logContainer.scrollTop = logContainer.scrollHeight;
  } catch (error) {
    console.error('Error parsing log message:', error);
  }
});
