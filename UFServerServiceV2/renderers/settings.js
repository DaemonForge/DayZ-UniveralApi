/**
 * renderers/settings.js
 * 
 * This script handles the dynamic behavior of the settings page:
 * - Toggling certificate fields (and disabling hidden ones).
 * - Dynamic addition/removal of list items for ServerAuth, RateLimitWhiteList, and Functions.
 * - Generation of complex auth tokens.
 * - Loading/saving configuration via IPC.
 * - Updated: Auth keys now include an inline text input to allow labeling each auth key. 
 *            The labels are stored in a separate array (ServerAuthLabels) that maintains
 *            a one-to-one correspondence with the ServerAuth array.
 */

let unsavedChanges = false;
document.getElementById('floatingSaveBtn').hidden = true;
const configForm = document.getElementById('configForm');

// Helper function to show notifications in our custom dialog instead of native alerts.
function showNotification(message) {
  const notificationDialog = document.getElementById('notificationDialog');
  const notificationMessage = document.getElementById('notificationMessage');
  notificationMessage.innerText = message;
  notificationDialog.showModal();
}

document.getElementById('notificationOK').addEventListener('click', () => {
  document.getElementById('notificationDialog').close();
});

configForm.addEventListener('input', () => {
  unsavedChanges = true;
  // Show Save and Cancel/Close buttons if hidden.
  document.getElementById('saveBtn').hidden = false;
  document.getElementById('cancelBtn').hidden = false;
  document.getElementById('floatingSaveBtn').hidden = false;
});

document.addEventListener('DOMContentLoaded', async () => {
  const certTypeSelect = document.getElementById('certType');
  const proxyConfigContainer = document.getElementById('proxyConfigContainer');

  // Toggle certificate fields and show proxy configuration container when "proxy" is selected.
  certTypeSelect.addEventListener('change', () => {
    const type = certTypeSelect.value;
    const ownCertFields = document.getElementById('ownCertFields');
    const letsEncryptFields = document.getElementById('letsEncryptFields');

    // Toggle Own Certificates fields.
    if (type === 'ownCert') {
      ownCertFields.style.display = 'block';
      ownCertFields.querySelectorAll('input, textarea').forEach(input => {
        input.disabled = false;
      });
    } else {
      ownCertFields.style.display = 'none';
      ownCertFields.querySelectorAll('input, textarea').forEach(input => {
        input.disabled = true;
      });
    }

    // Toggle Let's Encrypt fields.
    if (type === 'letsEncrypt') {
      letsEncryptFields.style.display = 'block';
      letsEncryptFields.querySelectorAll('input, textarea').forEach(input => input.disabled = false);
    } else {
      letsEncryptFields.style.display = 'none';
      letsEncryptFields.querySelectorAll('input, textarea').forEach(input => input.disabled = true);
    }
    if (type === 'proxy') {
      proxyConfigContainer.style.display = 'block';
    } else {
      proxyConfigContainer.style.display = 'none';
    }
  });

  /**
   * createAuthEntry: Creates a new list item for a ServerAuth token with an inline label.
   * The token is displayed in a read-only input; next to it a text input field allows a label.
   * Copy and delete buttons are included.
   *
   * @param {string} value - The auth token value.
   * @param {string} label - The label for the auth key (defaults to empty string).
   * @returns {HTMLElement} - The constructed list item element.
   */
  function createAuthEntry(value = '', label = '') {
    const wrapper = document.createElement('div');
    wrapper.className = 'list-item';

    // Auth token input (read-only). Added a specific class for later selection.
    const tokenInput = document.createElement('input');
    tokenInput.type = 'text';
    tokenInput.readOnly = true;
    tokenInput.value = value;
    tokenInput.title = "Auth token (read-only)";
    tokenInput.style.flex = "1";
    tokenInput.classList.add('auth-key-input');

    // Label input for the auth token. Users can edit this field.
    const labelInput = document.createElement('input');
    labelInput.type = 'text';
    labelInput.value = label;
    labelInput.placeholder = 'Label for auth key';
    labelInput.classList.add('auth-label-input');
    // Set a fixed width for label input (adjust as needed)
    labelInput.style.width = '150px';
    labelInput.style.marginLeft = '8px';
    // When label input is modified, mark unsaved changes.
    labelInput.addEventListener('input', () => {
      unsavedChanges = true;
      document.getElementById('saveBtn').hidden = false;
      document.getElementById('cancelBtn').hidden = false;
      document.getElementById('floatingSaveBtn').hidden = false;
    });

    // Copy button with provided SVG.
    const copyBtn = document.createElement('button');
    copyBtn.type = 'button';
    copyBtn.className = 'delete-btn copy-btn';
    copyBtn.title = 'Copy Auth Token';
    copyBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="0 0 24 24" width="24">
      <path d="M0 0h24v24H0z" fill="none"/>
      <path d="M16 1H4c-1.1 0-2 .9-2 2v14h2V3h12V1zm3 4H8c-1.1 0-2 .9-2 2v14c0 1.1.9 2 2 2h11c1.1 0 2-.9 2-2V7c0-1.1-.9-2-2-2zm0 16H8V7h11v14z"/>
    </svg>`;
    copyBtn.addEventListener('click', () => {
      navigator.clipboard.writeText(tokenInput.value)
        .then(() => {
          copyBtn.classList.add('animate-copy');
          copyBtn.title = 'Copied!';
          setTimeout(() => {
            copyBtn.classList.remove('animate-copy');
            copyBtn.title = 'Copy Auth Token';
          }, 2000);
        })
        .catch(err => {
          console.error("Error copying auth token: ", err);
        });
    });

    // Delete button with provided SVG.
    const deleteBtn = document.createElement('button');
    deleteBtn.type = 'button';
    deleteBtn.className = 'delete-btn';
    deleteBtn.title = 'Delete this auth token';
    deleteBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="0 0 24 24" width="24">
      <path d="M0 0h24v24H0z" fill="none"/>
      <path d="M7 11v2h10v-2H7zm5-9C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8z"/>
    </svg>`;
    deleteBtn.addEventListener('click', () => {
      wrapper.remove();
      unsavedChanges = true;
      document.getElementById('saveBtn').hidden = false;
      document.getElementById('cancelBtn').hidden = false;
      document.getElementById('floatingSaveBtn').hidden = false;
    });

    // Append elements in order: auth token, label input, copy and delete buttons.
    wrapper.appendChild(tokenInput);
    wrapper.appendChild(labelInput);
    wrapper.appendChild(copyBtn);
    wrapper.appendChild(deleteBtn);
    return wrapper;
  }

  /**
   * createListItem: Creates a list item for inputs (e.g. IP addresses) with a delete button.
   * @param {string} value - The initial value.
   * @param {string} placeholder - Placeholder text.
   * @returns {HTMLElement} - The constructed list item.
   */
  function createListItem(value = '', placeholder = '') {
    const wrapper = document.createElement('div');
    wrapper.className = 'list-item';

    const input = document.createElement('input');
    input.type = 'text';
    input.value = value;
    input.placeholder = placeholder;
    input.style.flex = "1";

    const deleteBtn = document.createElement('button');
    deleteBtn.type = 'button';
    deleteBtn.className = 'delete-btn';
    deleteBtn.title = 'Delete this item';
    deleteBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="0 0 24 24" width="24">
      <path d="M0 0h24v24H0z" fill="none"/>
      <path d="M7 11v2h10v-2H7zm5-9C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8z"/>
    </svg>`;
    deleteBtn.addEventListener('click', () => {
      wrapper.remove();
      unsavedChanges = true;
      document.getElementById('saveBtn').hidden = false;
      document.getElementById('cancelBtn').hidden = false;
      document.getElementById('floatingSaveBtn').hidden = false;
    });

    wrapper.appendChild(input);
    wrapper.appendChild(deleteBtn);
    return wrapper;
  }

  // ----------------- Proxy Configuration Section -----------------
  async function loadProxyDomains() {
    try {
      const domains = await window.api.getProxyDomains();
      const dropdown = document.getElementById('proxyDomain');
      dropdown.innerHTML = "";
      domains.forEach(domain => {
        const option = document.createElement('option');
        option.value = domain;
        option.text = domain;
        dropdown.appendChild(option);
      });
    } catch (err) {
      showNotification("Error loading proxy domains: " + err.message);
    }
  }
  await loadProxyDomains();

  document.getElementById('proxyDomain').addEventListener('change', () => {
    const selectedDomain = document.getElementById('proxyDomain').value;
    const registered = document.getElementById('proxySubdomain').value;
    const warningDiv = document.getElementById('proxyWarning');
    if (registered) {
      const currentDomain = registered.split('.').slice(1).join('.');
      if (currentDomain !== selectedDomain) {
        warningDiv.style.display = 'block';
        warningDiv.innerText = "Warning: Changing the primary domain will stop auto-renewal for the current proxy token.";
      } else {
        warningDiv.style.display = 'none';
        warningDiv.innerText = "";
      }
    }
  });

  async function registerProxy() {
    const selectedDomain = document.getElementById('proxyDomain').value;
    try {
      const data = await window.api.registerProxy(selectedDomain);
      // Update UI: hide the not-registered controls and show the registered container.
      document.getElementById('proxyNotRegistered').style.display = 'none';
      document.getElementById('proxyRegistered').style.display = 'flex';
      document.getElementById('proxySubdomain').value = data.subdomain;
      document.getElementById('proxyToken').value = data.token;
      // Auto save the config immediately upon successful proxy registration.
      let currentConfig = await window.api.getConfig();
      if (!currentConfig) currentConfig = {};
      currentConfig.Proxy = {
        primaryDomain: selectedDomain,
        subdomain: data.subdomain,
        token: data.token,
        lastRenew: new Date().toISOString()
      };
      const saveRes = await window.api.saveConfig(currentConfig);
      unsavedChanges = true;
      if (!saveRes.success) {
        showNotification("Proxy registered, but failed to auto-save configuration.");
        unsavedChanges = true;
      }
    } catch (err) {
      showNotification("Error registering proxy: " + err.message);
    }
  }
  document.getElementById('registerProxyBtn').addEventListener('click', registerProxy);

  document.getElementById('copyProxySubdomain').addEventListener('click', () => {
    const proxySub = document.getElementById('proxySubdomain').value;
    if (proxySub) {
      navigator.clipboard.writeText(proxySub)
        .then(() => {
          const btn = document.getElementById('copyProxySubdomain');
          btn.classList.add('animate-copy');
          btn.title = 'Copied!';
          setTimeout(() => {
            btn.classList.remove('animate-copy');
            btn.title = 'Copy Subdomain';
          }, 2000);
        })
        .catch(err => {
          console.error("Error copying proxy subdomain: ", err);
        });
    }
  });

  document.getElementById('deleteProxySubdomain').addEventListener('click', () => {
    const confirmDelete = confirm("Are you sure you want to delete the registered proxy subdomain? You will then need to register a new one.");
    if (confirmDelete) {
      document.getElementById('proxySubdomain').value = "";
      document.getElementById('proxyToken').value = "";
      document.getElementById('proxyRegistered').style.display = 'none';
      document.getElementById('proxyNotRegistered').style.display = 'flex';
      unsavedChanges = true;
      document.getElementById('saveBtn').hidden = false;
      document.getElementById('cancelBtn').hidden = false;
      document.getElementById('floatingSaveBtn').hidden = false;
    }
  });

  document.getElementById('addRateLimit').addEventListener('click', () => {
    const container = document.getElementById('rateLimitList');
    const item = createListItem('', 'IP Address');
    container.appendChild(item);
  });

  // Event listener for generating a new ServerAuth token.
  document.getElementById('generateServerAuth').addEventListener('click', () => {
    const newAuth = makeAuthToken();
    const container = document.getElementById('serverAuthList');
    // Pass an empty string for the label.
    const item = createAuthEntry(newAuth, '');
    container.appendChild(item);
    unsavedChanges = true;
    document.getElementById('saveBtn').hidden = false;
    document.getElementById('cancelBtn').hidden = false;
    document.getElementById('floatingSaveBtn').hidden = false;
  });

  // Event listener for adding a new Function entry.
  document.getElementById('addFunction').addEventListener('click', () => {
    const container = document.getElementById('functionsContainer');
    const div = document.createElement('div');
    div.className = 'functionEntry';
    // Horizontal layout for function entry, including delete button.
    div.innerHTML = `
      <input type="text" placeholder="Mod Name" class="modNameInput" required title="Enter module name">
      <div class="toggle-container" title="Allow DB operations">
        <label class="toggle-switch">
          <input type="checkbox" class="allowDB">
          <span class="slider"></span>
        </label>
        <span>DB</span>
      </div>

      <div class="toggle-container" title="Allow Discord Bot functions">
        <label class="toggle-switch">
          <input type="checkbox" class="allowDiscordBot">
          <span class="slider"></span>
        </label>
        <span>Discord</span>
      </div>
      <div class="toggle-container" title="Allow Message Queue">
        <label class="toggle-switch">
          <input type="checkbox" class="allowMsgQueue">
          <span class="slider"></span>
        </label>
        <span>MsgQueue</span>
      </div>
      <button type="button" class="delete-btn" title="Delete this function">
        <svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="0 0 24 24" width="24">
          <path d="M0 0h24v24H0z" fill="none"/>
          <path d="M7 11v2h10v-2H7zm5-9C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8z"/>
        </svg>
      </button>
    `;
    div.querySelector('.delete-btn').addEventListener('click', () => { 
      unsavedChanges = true;
      document.getElementById('saveBtn').hidden = false;
      document.getElementById('cancelBtn').hidden = false;
      document.getElementById('floatingSaveBtn').hidden = false;
      div.remove(); 
    });
    container.appendChild(div);
  });

  function makeAuthToken() {
    let result = '';
    const characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!~';
    for (let i = 0; i < 48; i++) {
      result += characters.charAt(Math.floor(Math.random() * characters.length));
    }
    return result;
  }

  // ----------------- Load and Populate Config -----------------
  async function loadConfig() {
    const config = await window.api.getConfig();
    const cfg = config || {
      DBServer: "mongodb://localhost:27017",
      DB: "DayZ",
      CreateIndexes: true,
      IP: "0.0.0.0",
      Port: 443,
      RateLimitWhiteList: ["127.0.0.1"],
      ServerAuth: [makeAuthToken()],
      ServerAuthLabels: [""],
      Discord: {
        Client_Id: "",
        Client_Secret: "",
        Bot_Token: "",
        Guild_Id: "",
        AllowToReRegister: false,
        Restrict_Sign_Up: false,
        Required_Role: "",
        BlackList_Role: "",
        Restrict_Sign_Up_Countries: []
      },
      OpenAIApi: {
        ApiKey: "",
        enablePromptProtection: true
      },
      Functions: {
        "ExampleMod": {
          AllowDB: false,
          AllowDiscordBot: false,
          AllowMsgQueue: false
        }
      },
      LogToFile: true,
      CheckForNewVersion: true,
      Certificate: "",
      CertificateKey: "",
      LetsEncypt: {
        Enabled: false,
        Domain: "localhost.localhost",
        Email: "myemail@email.com",
        AltNames: []
      },
      Proxy: {
        primaryDomain: "",
        subdomain: "",
        token: "",
        lastRenew: ""
      }
    };

    document.getElementById('DBServer').value = cfg.DBServer;
    document.getElementById('DB').value = cfg.DB;
    document.getElementById('CreateIndexes').checked = cfg.CreateIndexes;
    document.getElementById('IP').value = cfg.IP;
    document.getElementById('Port').value = cfg.Port;

    const rateList = document.getElementById('rateLimitList');
    rateList.innerHTML = "";
    cfg.RateLimitWhiteList.forEach(ip => {
      const item = createListItem(ip, 'IP Address');
      rateList.appendChild(item);
    });

    const authList = document.getElementById('serverAuthList');
    authList.innerHTML = "";
    (cfg.ServerAuth || []).forEach((auth, index) => {
      const labelValue = (cfg.ServerAuthLabels && Array.isArray(cfg.ServerAuthLabels)) ? (cfg.ServerAuthLabels[index] || '') : '';
      const item = createAuthEntry(auth, labelValue);
      authList.appendChild(item);
    });

    let certType = 'selfSigned';
    if (cfg.LetsEncypt.Enabled) {
      certType = 'letsEncrypt';
    } else if (cfg.Certificate !== "" || cfg.CertificateKey !== "") {
      certType = 'ownCert';
    } else if (cfg.Proxy && cfg.Proxy.subdomain) {
      certType = 'proxy';
    }
    document.getElementById('certType').value = certType;
    certTypeSelect.dispatchEvent(new Event('change'));
    document.getElementById('Certificate').value = cfg.Certificate;
    document.getElementById('CertificateKey').value = cfg.CertificateKey;
    document.getElementById('LE_Domain').value = cfg.LetsEncypt.Domain;
    document.getElementById('LE_Email').value = cfg.LetsEncypt.Email;
    document.getElementById('LE_AltNames').value = cfg.LetsEncypt.AltNames.join(', ');

    document.getElementById('Discord_Client_Id').value = cfg.Discord.Client_Id;
    document.getElementById('Discord_Client_Secret').value = cfg.Discord.Client_Secret;
    document.getElementById('Discord_Bot_Token').value = cfg.Discord.Bot_Token;
    document.getElementById('Discord_Guild_Id').value = cfg.Discord.Guild_Id;
    document.getElementById('Discord_AllowToReRegister').checked = cfg.Discord.AllowToReRegister;
    document.getElementById('Discord_Restrict_Sign_Up').checked = cfg.Discord.Restrict_Sign_Up;
    document.getElementById('Discord_Required_Role').value = cfg.Discord.Required_Role;
    document.getElementById('Discord_BlackList_Role').value = cfg.Discord.BlackList_Role;
    document.getElementById('Restrict_Sign_Up_Countries').value = Array.isArray(cfg.Discord.Restrict_Sign_Up_Countries)
      ? cfg.Discord.Restrict_Sign_Up_Countries.join(', ')
      : cfg.Discord.Restrict_Sign_Up_Countries;

    document.getElementById('OpenAIApi_ApiKey').value = cfg.OpenAIApi.ApiKey;
    document.getElementById('OpenAIApi_enablePromptProtection').checked = cfg.OpenAIApi.enablePromptProtection;

    const funcContainer = document.getElementById('functionsContainer');
    funcContainer.innerHTML = "";
    for (const mod in cfg.Functions) {
      const div = document.createElement('div');
      div.className = 'functionEntry';
      const fun = cfg.Functions[mod];
      div.innerHTML = `
        <input type="text" value="${mod}" class="modNameInput" required title="Module Name">
        <div class="toggle-container" title="Allow DB operations">
          <label class="toggle-switch">
            <input type="checkbox" ${fun.AllowDB ? 'checked' : ''} class="allowDB">
            <span class="slider"></span>
          </label>
          <span>DB</span>
        </div>
        <div class="toggle-container" title="Allow Discord Bot functions">
          <label class="toggle-switch">
            <input type="checkbox" ${fun.AllowDiscordBot ? 'checked' : ''} class="allowDiscordBot">
            <span class="slider"></span>
          </label>
          <span>Discord</span>
        </div>
        <div class="toggle-container" title="Allow Message Queue">
          <label class="toggle-switch">
            <input type="checkbox" ${fun.AllowMsgQueue ? 'checked' : ''} class="allowMsgQueue">
            <span class="slider"></span>
          </label>
          <span>MsgQueue</span>
        </div>
        <button type="button" class="delete-btn" title="Delete this function">
          <svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="0 0 24 24" width="24">
            <path d="M0 0h24v24H0z" fill="none"/>
            <path d="M7 11v2h10v-2H7zm5-9C6.48 2 2 6.48 2 12s4.48 10 10 10 10-4.48 10-10S17.52 2 12 2zm0 18c-4.41 0-8-3.59-8-8s3.59-8 8-8 8 3.59 8 8-3.59 8-8 8z"/>
          </svg>
        </button>
      `;
      div.querySelector('.delete-btn').addEventListener('click', () => { 
        unsavedChanges = true;
        document.getElementById('saveBtn').hidden = false;
        document.getElementById('cancelBtn').hidden = false;
        document.getElementById('floatingSaveBtn').hidden = false;
        div.remove(); 
      });
      funcContainer.appendChild(div);
    }

    // Populate Proxy configuration fields.
    if (cfg.Proxy) {
      document.getElementById('proxySubdomain').value = cfg.Proxy.subdomain || "";
      if (cfg.Proxy.primaryDomain) {
        const dropdown = document.getElementById('proxyDomain');
        dropdown.innerHTML = "";
        const option = document.createElement('option');
        option.value = cfg.Proxy.primaryDomain;
        option.text = cfg.Proxy.primaryDomain;
        dropdown.appendChild(option);
        dropdown.value = cfg.Proxy.primaryDomain;
      }
      if (cfg.Proxy.subdomain) {
        document.getElementById('proxyNotRegistered').style.display = 'none';
        document.getElementById('proxyRegistered').style.display = 'flex';
      } else {
        document.getElementById('proxyNotRegistered').style.display = 'flex';
        document.getElementById('proxyRegistered').style.display = 'none';
      }
    }
  }
  await loadConfig();

  configForm.addEventListener('submit', async function(e) {
    e.preventDefault();
    
    const authItems = document.querySelectorAll('#serverAuthList .list-item');
    const serverAuth = [];
    const serverAuthLabels = [];
    authItems.forEach(item => {
      const keyInput = item.querySelector('.auth-key-input');
      const labelInput = item.querySelector('.auth-label-input');
      serverAuth.push(keyInput ? keyInput.value : '');
      serverAuthLabels.push(labelInput ? labelInput.value : '');
    });

    // If certificate type is "proxy", treat it like selfSigned (no certificates)
    const certType = document.getElementById('certType').value;
    let certificate = "";
    let certificateKey = "";
    if (certType === 'ownCert') {
      certificate = document.getElementById('Certificate').value;
      certificateKey = document.getElementById('CertificateKey').value;
    }
    const newConfig = {
      DBServer: document.getElementById('DBServer').value,
      DB: document.getElementById('DB').value,
      CreateIndexes: document.getElementById('CreateIndexes').checked,
      IP: document.getElementById('IP').value,
      Port: Number(document.getElementById('Port').value),
      RateLimitWhiteList: Array.from(document.querySelectorAll('#rateLimitList input')).map(input => input.value),
      ServerAuth: serverAuth,
      ServerAuthLabels: serverAuthLabels,
      Certificate: certificate,
      CertificateKey: certificateKey,
      Discord: {
        Client_Id: document.getElementById('Discord_Client_Id').value,
        Client_Secret: document.getElementById('Discord_Client_Secret').value,
        Bot_Token: document.getElementById('Discord_Bot_Token').value,
        Guild_Id: document.getElementById('Discord_Guild_Id').value,
        AllowToReRegister: document.getElementById('Discord_AllowToReRegister').checked,
        Restrict_Sign_Up: document.getElementById('Discord_Restrict_Sign_Up').checked,
        Required_Role: document.getElementById('Discord_Required_Role').value,
        BlackList_Role: document.getElementById('Discord_BlackList_Role').value,
        Restrict_Sign_Up_Countries: document.getElementById('Restrict_Sign_Up_Countries').value
          .split(',')
          .map(s => s.trim())
          .filter(s => s !== '')
      },
      OpenAIApi: {
        ApiKey: document.getElementById('OpenAIApi_ApiKey').value,
        enablePromptProtection: document.getElementById('OpenAIApi_enablePromptProtection').checked
      },
      Functions: {},
      LogToFile: document.getElementById('LogToFile').checked,
      CheckForNewVersion: document.getElementById('CheckForNewVersion').checked,
      LetsEncypt: {
        Enabled: document.getElementById('certType').value === 'letsEncrypt',
        Domain: document.getElementById('LE_Domain').value,
        Email: document.getElementById('LE_Email').value,
        AltNames: document.getElementById('LE_AltNames').value
          .split(',')
          .map(s => s.trim())
          .filter(s => s !== '')
      },
      Proxy: {
        primaryDomain: document.getElementById('certType').value === 'Proxy' ? document.getElementById('proxyDomain').value : "",
        subdomain: document.getElementById('certType').value === 'Proxy' ?  document.getElementById('proxySubdomain').value : "",
        token: document.getElementById('certType').value === 'Proxy' ?  document.getElementById('proxyToken').value : "",
        lastRenew: document.getElementById('certType').value === 'Proxy' ?  new Date().toISOString() : ""
      }
    };

    const functionEntries = document.querySelectorAll('.functionEntry');
    functionEntries.forEach(entry => {
      const modName = entry.querySelector('.modNameInput').value;
      const allowDB = entry.querySelector('.allowDB').checked;
      const allowDiscordBot = entry.querySelector('.allowDiscordBot').checked;
      const allowMsgQueue = entry.querySelector('.allowMsgQueue').checked;
      if (modName) {
        newConfig.Functions[modName] = {
          AllowDB: allowDB,
          AllowDiscordBot: allowDiscordBot,
          AllowMsgQueue: allowMsgQueue
        };
      }
    });

    const result = await window.api.saveConfig(newConfig);
    if (result.success) {
      unsavedChanges = false;
      document.getElementById('saveBtn').hidden = true;
      document.getElementById('cancelBtn').hidden = true;
      document.getElementById('floatingSaveBtn').hidden = true;
      const dialog = document.getElementById('saveDialog');
      dialog.showModal();
      document.getElementById('dialogClose').addEventListener('click', () => {
        dialog.close();
        window.api.forceClose();
        window.api.restartApp();
      }, { once: true });
      document.getElementById('dialogContinue').addEventListener('click', () => {
        dialog.close();
      }, { once: true });
    } else {
      showNotification("Failed to save configuration");
    }
  });

  window.api.onAttemptClose((event, ...args) => {
    if (unsavedChanges) {
      const confirmDialog = document.getElementById('confirmCloseDialog');
      confirmDialog.showModal();
    } else {
      window.api.forceClose();
    }
  });

  document.getElementById('cancelBtn').addEventListener('click', function() {
    if (unsavedChanges) {
      const confirmDialog = document.getElementById('confirmCloseDialog');
      confirmDialog.showModal();
    } else {
      window.api.forceClose();
    }
  });

  document.getElementById('floatingSaveBtn').addEventListener('click', () => {
    document.getElementById('saveBtn').click();
  });
});

document.getElementById('confirmCloseYes').addEventListener('click', () => {
  unsavedChanges = false;
  document.getElementById('confirmCloseDialog').close();
  window.api.forceClose();
});

document.getElementById('confirmCloseNo').addEventListener('click', () => {
  document.getElementById('confirmCloseDialog').close();
});