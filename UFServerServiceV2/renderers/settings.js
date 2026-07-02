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
let cachedConfig = null;
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

  // ----------------- Let's Encrypt Domain Status Check -----------------
  const leDomainCheckBtn = document.getElementById('leDomainCheckBtn');
  const leDomainStatusIndicator = document.getElementById('leDomainStatusIndicator');
  const leDomainStatusLabel = document.getElementById('leDomainStatusLabel');

  async function checkLEDomainStatus() {
    const domain = document.getElementById('LE_Domain')?.value?.trim();
    if (!domain) {
      leDomainStatusIndicator.textContent = '⚫';
      leDomainStatusLabel.textContent = 'No domain entered';
      return;
    }
    leDomainStatusIndicator.textContent = '🟡';
    leDomainStatusLabel.textContent = 'Checking...';
    leDomainCheckBtn.disabled = true;
    try {
      const result = await window.api.checkDomainStatus(domain);
      if (result.reachable && result.status === 'Success') {
        leDomainStatusIndicator.textContent = '🟢';
        leDomainStatusLabel.textContent = 'Online' + (result.version ? ` (v${result.version})` : '');
      } else if (result.reachable) {
        leDomainStatusIndicator.textContent = '🟡';
        leDomainStatusLabel.textContent = 'Reachable but status: ' + (result.status || result.error || 'Unknown');
      } else {
        leDomainStatusIndicator.textContent = '🔴';
        leDomainStatusLabel.textContent = 'Unreachable — ' + (result.error || 'Unknown error');
      }
    } catch (err) {
      leDomainStatusIndicator.textContent = '🔴';
      leDomainStatusLabel.textContent = 'Check failed';
    } finally {
      leDomainCheckBtn.disabled = false;
    }
  }

  if (leDomainCheckBtn) {
    leDomainCheckBtn.addEventListener('click', checkLEDomainStatus);
  }

  /**
   * resolveServerURL: Determines the best ServerURL for the UFramework.json DayZ config.
   * Priority: Tunnel hostname > Proxy subdomain > Let's Encrypt domain > fallback localhost:Port.
   * @returns {Promise<string>} The resolved server URL (e.g. "https://example.com/").
   */
  async function resolveServerURL() {
    // 1. Tunnel hostname (highest priority)
    try {
      const status = await window.api.tunnelStatus();
      if (status && status.hostname) {
        return 'https://' + status.hostname + '/';
      }
    } catch (_) { /* tunnel not available */ }

    // 2. Proxy subdomain
    const proxySub = document.getElementById('proxySubdomain')?.value?.trim();
    if (proxySub) {
      return 'https://' + proxySub + '/';
    }

    // 3. Let's Encrypt domain
    const leDomain = document.getElementById('LE_Domain')?.value?.trim();
    if (leDomain) {
      return 'https://' + leDomain + '/';
    }

    // 4. Fallback: localhost with configured port
    const port = document.getElementById('Port')?.value || '443';
    return 'https://localhost:' + port + '/';
  }

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

    // "Copy DayZ Config" button — generates the UFramework.json content for this auth key
    const configBtn = document.createElement('button');
    configBtn.type = 'button';
    configBtn.className = 'delete-btn copy-btn';
    configBtn.title = 'Copy DayZ Server Config (UFramework.json)';
    configBtn.innerHTML = `<svg xmlns="http://www.w3.org/2000/svg" height="24" viewBox="0 0 24 24" width="24">
      <path d="M0 0h24v24H0z" fill="none"/>
      <path d="M14 2H6c-1.1 0-2 .9-2 2v16c0 1.1.9 2 2 2h12c1.1 0 2-.9 2-2V8l-6-6zm-1 7V3.5L18.5 9H13zM6 20V4h5v7h7v9H6z"/>
    </svg>`;
    configBtn.addEventListener('click', async () => {
      try {
        const serverUrl = await resolveServerURL();
        const serverId = labelInput.value?.trim() || '';
        const discordEnabled = !!document.getElementById('Discord_Bot_Token')?.value?.trim();
        const config = {
          ConfigVersion: "2",
          ServerURL: serverUrl,
          ServerID: serverId,
          ServerAuth: tokenInput.value,
          EnableBuiltinLogging: 0,
          PromptDiscordOnConnect: discordEnabled ? 1 : 0,
          DebugLevel: "INFO",
          LogToSeperateFile: 0
        };
        await navigator.clipboard.writeText(JSON.stringify(config, null, 4));
        configBtn.classList.add('animate-copy');
        configBtn.title = 'Config Copied!';
        showNotification('UFramework.json config copied to clipboard');
        setTimeout(() => {
          configBtn.classList.remove('animate-copy');
          configBtn.title = 'Copy DayZ Server Config (UFramework.json)';
        }, 2000);
      } catch (err) {
        console.error('Error generating DayZ config:', err);
        showNotification('Failed to generate config: ' + err.message);
      }
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

    // Append elements in order: auth token, label input, copy config, copy auth, and delete buttons.
    wrapper.appendChild(tokenInput);
    wrapper.appendChild(labelInput);
    wrapper.appendChild(configBtn);
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
    const dropdown = document.getElementById('proxyDomain');
    if (!dropdown) {
      return;
    }
    try {
      const result = await window.api.getProxyDomains();
      const domains = Array.isArray(result) ? result : (result?.domains || []);
      const errorMessage = Array.isArray(result) ? null : (result?.error || null);
      dropdown.innerHTML = "";
      if (domains.length) {
        domains.forEach(domain => {
          const option = document.createElement('option');
          option.value = domain;
          option.text = domain;
          dropdown.appendChild(option);
        });
      } else {
        const option = document.createElement('option');
        option.value = "";
        option.text = errorMessage ? 'No proxy domains available' : 'No domains returned';
        option.disabled = true;
        option.selected = true;
        dropdown.appendChild(option);
      }

      if (errorMessage) {
        dropdown.title = `Unable to load proxy domains: ${errorMessage}`;
        dropdown.classList.add('proxy-dropdown-error');
        showNotification("Error loading proxy domains: " + errorMessage);
      } else {
        dropdown.title = 'Select the primary domain for your proxy.';
        dropdown.classList.remove('proxy-dropdown-error');
      }
    } catch (err) {
      dropdown.innerHTML = "";
      const option = document.createElement('option');
      option.value = "";
      option.text = 'Failed to fetch proxy domains';
      option.disabled = true;
      option.selected = true;
      dropdown.appendChild(option);
      dropdown.title = 'Unable to load proxy domains: ' + err.message;
      dropdown.classList.add('proxy-dropdown-error');
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
      if (saveRes.success) {
        cachedConfig = JSON.parse(JSON.stringify(currentConfig));
      } else {
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
    cachedConfig = config ? JSON.parse(JSON.stringify(config)) : null;
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
        BaseURL: "",
        DefaultModel: "",
        EmbeddingModel: "",
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
      },
      Tunnel: {
        enabled: false,
        token: "",
        autoStart: true
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

    if (!cachedConfig) {
      cachedConfig = JSON.parse(JSON.stringify(cfg));
    }
    document.getElementById('certType').value = certType;
    certTypeSelect.dispatchEvent(new Event('change'));
    document.getElementById('Certificate').value = cfg.Certificate;
    document.getElementById('CertificateKey').value = cfg.CertificateKey;
    document.getElementById('LE_Domain').value = cfg.LetsEncypt.Domain;
    document.getElementById('LE_Email').value = cfg.LetsEncypt.Email;
    document.getElementById('LE_AltNames').value = cfg.LetsEncypt.AltNames.join(', ');

    // Auto-check LE domain status if Let's Encrypt is enabled and a domain is configured
    if (cfg.LetsEncypt.Enabled && cfg.LetsEncypt.Domain) {
      checkLEDomainStatus();
    }

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
    document.getElementById('OpenAIApi_BaseURL').value = cfg.OpenAIApi.BaseURL || "";
    document.getElementById('OpenAIApi_DefaultModel').value = cfg.OpenAIApi.DefaultModel || "";
    document.getElementById('OpenAIApi_EmbeddingModel').value = cfg.OpenAIApi.EmbeddingModel || "";
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

    // Populate Tunnel configuration fields.
    const tunnel = cfg.Tunnel || {};
    document.getElementById('Tunnel_enabled').checked = Boolean(tunnel.enabled);
    document.getElementById('Tunnel_token').value = tunnel.token || '';
    document.getElementById('Tunnel_autoStart').checked = tunnel.autoStart !== false;
    toggleTunnelFields(Boolean(tunnel.enabled));
  }
  await loadConfig();

  // ----------------- External Links Handler -----------------
  // Make all links with class "ext-link" open in the system browser
  document.addEventListener('click', (e) => {
    const link = e.target.closest('a.ext-link');
    if (link && link.href) {
      e.preventDefault();
      if (window.api && window.api.openExternal) {
        window.api.openExternal(link.href);
      }
    }
  });

  // ----------------- Tunnel Token Stripping -----------------
  /**
   * Strip common prefixes from pasted tunnel tokens.
   * Users often paste the full command: "cloudflared.exe service install eyJ..."
   * We want just the token part: "eyJ..."
   */
  function stripTunnelToken(value) {
    if (!value) return value;
    let stripped = value.trim();
    // Remove common command prefixes (case-insensitive)
    stripped = stripped.replace(/^.*(?:cloudflared(?:\.exe)?\s+(?:service\s+install|tunnel\s+(?:--no-autoupdate\s+)?run\s+--token))\s+/i, '');
    // Also handle if they pasted with quotes
    stripped = stripped.replace(/^["']+|["']+$/g, '');
    return stripped.trim();
  }

  const tunnelTokenInput = document.getElementById('Tunnel_token');
  const tunnelTokenStripped = document.getElementById('tunnelTokenStripped');

  function handleTokenClean() {
    const raw = tunnelTokenInput.value;
    const clean = stripTunnelToken(raw);
    if (clean !== raw) {
      tunnelTokenInput.value = clean;
      tunnelTokenStripped.style.display = '';
      setTimeout(() => { tunnelTokenStripped.style.display = 'none'; }, 4000);
    }
  }

  tunnelTokenInput.addEventListener('paste', () => {
    // Defer to let the paste value populate the input first
    setTimeout(handleTokenClean, 0);
  });
  tunnelTokenInput.addEventListener('blur', handleTokenClean);

  // ----------------- Tunnel Setup Guide Dialog -----------------
  const tunnelSetupDialog = document.getElementById('tunnelSetupDialog');
  document.getElementById('tunnelSetupGuideBtn').addEventListener('click', () => {
    // Update dynamic values in the guide before showing
    const port = document.getElementById('Port').value || '443';
    const serviceUrlEl = document.getElementById('guideServiceUrl');
    const modUrlEl = document.getElementById('guideModUrl');
    const currentPortEl = document.getElementById('guideCurrentPort');
    if (serviceUrlEl) serviceUrlEl.textContent = 'localhost:' + port;
    if (currentPortEl) currentPortEl.textContent = port;
    if (modUrlEl) modUrlEl.textContent = 'https://api.yourdomain.com';
    tunnelSetupDialog.showModal();
  });
  document.getElementById('tunnelSetupClose').addEventListener('click', () => {
    tunnelSetupDialog.close();
  });

  // ----------------- Tunnel Configuration Section -----------------
  function toggleTunnelFields(enabled) {
    const fields = document.getElementById('tunnelConfigFields');
    if (fields) {
      fields.style.opacity = enabled ? '1' : '0.5';
      fields.querySelectorAll('input:not(#Tunnel_enabled), button').forEach(el => {
        el.disabled = !enabled;
      });
    }
  }

  document.getElementById('Tunnel_enabled').addEventListener('change', (e) => {
    toggleTunnelFields(e.target.checked);
  });

  function updateTunnelStatusUI(status) {
    const indicator = document.getElementById('tunnelStatusIndicator');
    const label = document.getElementById('tunnelStatusLabel');
    const startBtn = document.getElementById('tunnelStartBtn');
    const stopBtn = document.getElementById('tunnelStopBtn');
    const errorsDiv = document.getElementById('tunnelErrors');
    const versionLabel = document.getElementById('tunnelVersionLabel');
    const updateBadge = document.getElementById('tunnelUpdateBadge');
    const updateBtn = document.getElementById('tunnelUpdateBtn');
    const domainRow = document.getElementById('tunnelDomainRow');
    const domainLabel = document.getElementById('tunnelDomainLabel');

    if (!indicator) return;

    const hasErrors = status.errors && status.errors.length > 0;

    if (status.downloading) {
      indicator.textContent = '⏳';
      label.textContent = 'Downloading cloudflared...';
      startBtn.style.display = 'none';
      stopBtn.style.display = 'none';
    } else if (status.running) {
      if (status.connectedAt) {
        indicator.textContent = '🟢';
        const domainSuffix = status.hostname ? ` — ${status.hostname}` : (status.url ? ` — ${status.url}` : '');
        label.textContent = 'Online' + domainSuffix;
      } else if (hasErrors) {
        indicator.textContent = '⚠️';
        label.textContent = 'Error';
      } else {
        indicator.textContent = '🟡';
        label.textContent = 'Connecting...';
      }
      startBtn.style.display = 'none';
      stopBtn.style.display = '';
    } else {
      indicator.textContent = '🔴';
      label.textContent = 'Offline';
      startBtn.style.display = '';
      stopBtn.style.display = 'none';
    }

    // Version info
    if (versionLabel) {
      const ver = status.installedVersion || 'unknown';
      const latest = status.latestVersion ? ` (latest: ${status.latestVersion})` : '';
      versionLabel.textContent = `Version: ${ver}${latest}`;
    }
    if (updateBadge && updateBtn) {
      if (status.updateAvailable) {
        updateBadge.style.display = '';
        updateBtn.style.display = '';
      } else {
        updateBadge.style.display = 'none';
        updateBtn.style.display = 'none';
      }
    }

    if (status.errors && status.errors.length > 0) {
      errorsDiv.style.display = 'block';
      errorsDiv.textContent = status.errors.slice(-3).join('\n');
    } else {
      errorsDiv.style.display = 'none';
      errorsDiv.textContent = '';
    }

    // Domain/hostname display
    if (domainRow && domainLabel) {
      const host = status.hostname || status.url || null;
      if (host && status.running) {
        domainRow.style.display = '';
        domainLabel.textContent = host;
      } else {
        domainRow.style.display = 'none';
        domainLabel.textContent = '';
      }
    }
  }

  // Load initial tunnel status
  try {
    const initialStatus = await window.api.tunnelStatus();
    updateTunnelStatusUI(initialStatus);
  } catch (_) { /* ignore */ }

  // Listen for live status updates
  window.api.onTunnelStatusChanged((status) => {
    updateTunnelStatusUI(status);
  });

  document.getElementById('tunnelStartBtn').addEventListener('click', async () => {
    const tokenInput = document.getElementById('Tunnel_token');
    // Clean the token in case they pasted the full command
    handleTokenClean();
    if (!tokenInput.value) {
      showNotification('Please enter a Cloudflare Tunnel token first. Click the Setup Guide button for instructions.');
      return;
    }
    const result = await window.api.tunnelStart();
    if (!result.success) {
      // Check if the error is about missing token in saved config
      if (result.error && result.error.includes('No tunnel token configured')) {
        showNotification('Your tunnel token has not been saved yet. Please click the Save button first, then restart the app and try again.');
      } else {
        showNotification('Failed to start tunnel: ' + result.error);
      }
    }
  });

  document.getElementById('tunnelStopBtn').addEventListener('click', async () => {
    const result = await window.api.tunnelStop();
    if (!result.success) {
      showNotification('Failed to stop tunnel: ' + result.error);
    }
  });

  document.getElementById('tunnelDownloadBtn').addEventListener('click', async () => {
    const btn = document.getElementById('tunnelDownloadBtn');
    btn.disabled = true;
    btn.textContent = 'Downloading...';
    try {
      const result = await window.api.tunnelDownload();
      if (result.success) {
        showNotification(result.message || 'cloudflared downloaded successfully!');
      } else {
        showNotification('Download failed: ' + result.error);
      }
    } catch (err) {
      showNotification('Download error: ' + err.message);
    } finally {
      btn.disabled = false;
      btn.innerHTML = `<svg style="fill: #ffffff;" xmlns="http://www.w3.org/2000/svg" height="20" viewBox="0 0 24 24" width="20"><path d="M0 0h24v24H0z" fill="none"/><path d="M19 9h-4V3H9v6H5l7 7 7-7zM5 18v2h14v-2H5z"/></svg> Download cloudflared`;
    }
  });

  document.getElementById('tunnelCheckUpdateBtn').addEventListener('click', async () => {
    const btn = document.getElementById('tunnelCheckUpdateBtn');
    btn.disabled = true;
    btn.textContent = '🔄 Checking...';
    try {
      const result = await window.api.tunnelCheckUpdate();
      if (result.success) {
        if (result.updateAvailable) {
          showNotification(`Update available: ${result.latestVersion} (installed: ${result.installedVersion})`);
        } else {
          showNotification(`cloudflared is up to date (${result.installedVersion || 'unknown'})`);
        }
      } else {
        showNotification('Update check failed: ' + result.error);
      }
    } catch (err) {
      showNotification('Update check error: ' + err.message);
    } finally {
      btn.disabled = false;
      btn.textContent = '🔄 Check for Update';
    }
  });

  document.getElementById('tunnelUpdateBtn').addEventListener('click', async () => {
    const btn = document.getElementById('tunnelUpdateBtn');
    btn.disabled = true;
    btn.textContent = '⬆️ Updating...';
    try {
      const result = await window.api.tunnelUpdate();
      if (result.success) {
        showNotification('cloudflared updated successfully!');
      } else {
        showNotification('Update failed: ' + result.error);
      }
    } catch (err) {
      showNotification('Update error: ' + err.message);
    } finally {
      btn.disabled = false;
      btn.textContent = '⬆️ Update Now';
    }
  });

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
  const isProxyCert = certType === 'proxy';
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
        BaseURL: document.getElementById('OpenAIApi_BaseURL').value.trim(),
        DefaultModel: document.getElementById('OpenAIApi_DefaultModel').value.trim(),
        EmbeddingModel: document.getElementById('OpenAIApi_EmbeddingModel').value.trim(),
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
      Proxy: (() => {
        const previousProxy = (cachedConfig && cachedConfig.Proxy) ? cachedConfig.Proxy : {};
        if (!isProxyCert) {
          return {
            primaryDomain: "",
            subdomain: "",
            token: "",
            lastRenew: "",
            autoRenew: false
          };
        }
        return {
          primaryDomain: document.getElementById('proxyDomain').value,
          subdomain: document.getElementById('proxySubdomain').value,
          token: document.getElementById('proxyToken').value,
          lastRenew: previousProxy.lastRenew || new Date().toISOString(),
          autoRenew: Boolean(previousProxy.autoRenew)
        };
      })(),
      Tunnel: {
        enabled: document.getElementById('Tunnel_enabled').checked,
        token: stripTunnelToken(document.getElementById('Tunnel_token').value),
        autoStart: document.getElementById('Tunnel_autoStart').checked
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
      cachedConfig = JSON.parse(JSON.stringify(newConfig));
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