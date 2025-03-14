/**
 * renderers/settings.js
 * 
 * This script handles the dynamic behavior of the settings page:
 * - Toggling certificate fields (and disabling hidden ones).
 * - Dynamic addition/removal of list items for ServerAuth, RateLimitWhiteList, and Functions.
 * - Generation of complex auth tokens.
 * - Loading/saving configuration via IPC.
 * - Updated: Discord restriction fields are now located in the Advanced section.
 */

document.addEventListener('DOMContentLoaded', async () => {
    const certTypeSelect = document.getElementById('certType');
  
    // Toggle certificate fields based on selected certificate type.
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
        letsEncryptFields.querySelectorAll('input, textarea').forEach(input => {
          input.disabled = false;
        });
      } else {
        letsEncryptFields.style.display = 'none';
        letsEncryptFields.querySelectorAll('input, textarea').forEach(input => {
          input.disabled = true;
        });
      }
    });
  
    /**
     * createAuthEntry: Creates a new list item for a ServerAuth token.
     * The token is displayed in a read-only input with copy and delete buttons.
     * @param {string} value - The auth token value.
     * @returns {HTMLElement} - The constructed list item element.
     */
    function createAuthEntry(value = '') {
      const wrapper = document.createElement('div');
      wrapper.className = 'list-item';
      
      const input = document.createElement('input');
      input.type = 'text';
      input.readOnly = true;
      input.value = value;
      input.title = "Auth token (read-only)";
      input.style.flex = "1";
      
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
        navigator.clipboard.writeText(input.value);
        copyBtn.title = 'Copied!';
        setTimeout(() => { copyBtn.title = 'Copy Auth Token'; }, 2000);
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
      });
      
      wrapper.appendChild(input);
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
      });
      
      wrapper.appendChild(input);
      wrapper.appendChild(deleteBtn);
      return wrapper;
    }
    
    // Event listener for adding a new IP address entry.
    document.getElementById('addRateLimit').addEventListener('click', () => {
      const container = document.getElementById('rateLimitList');
      const item = createListItem('', 'IP Address');
      container.appendChild(item);
    });
    
    
    // Event listener for generating a new ServerAuth token.
    document.getElementById('generateServerAuth').addEventListener('click', () => {
      const newAuth = makeAuthToken();
      const container = document.getElementById('serverAuthList');
      const item = createAuthEntry(newAuth);
      container.appendChild(item);
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
    <input type="checkbox"  class="allowDiscordBot">
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
    div.querySelector('.delete-btn').addEventListener('click', () => { div.remove(); });
    container.appendChild(div);
  });
  
  /**
   * Generates a random 48-character authentication token.
   * @returns {string} A complex 48-character token.
   */
  function makeAuthToken() {
    let result = '';
    const characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-.!~';
    const charactersLength = characters.length;
    for (let i = 0; i < 48; i++) {
      result += characters.charAt(Math.floor(Math.random() * charactersLength));
    }
    return result;
  }
    
    /**
     * loadConfig: Loads the current configuration via IPC and populates the form fields.
     * Uses fallback defaults if no config is available.
     */
    async function loadConfig() {
        const config = await window.api.getConfig();
        // Use fallback defaults if no config exists.
        const cfg = config || {
          DBServer: "mongodb://localhost:27017",
          DB: "DayZ",
          CreateIndexes: true,
          IP: "0.0.0.0",
          Port: 443,
          RateLimitWhiteList: ["127.0.0.1"],
          ServerAuth: [makeAuthToken()], // Generate a new token by default.
          Discord: {
            Client_Id: "",
            Client_Secret: "",
            Bot_Token: "",
            Guild_Id: "",
            AllowToReRegister: false,
            // These restrictions are now in advanced.
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
          }
        };
      
        // Populate Database section.
        document.getElementById('DBServer').value = cfg.DBServer;
        document.getElementById('DB').value = cfg.DB;
        document.getElementById('CreateIndexes').checked = cfg.CreateIndexes;
        document.getElementById('IP').value = cfg.IP;
        document.getElementById('Port').value = cfg.Port;
      
        // Populate RateLimitWhiteList.
        const rateList = document.getElementById('rateLimitList');
        rateList.innerHTML = "";
        cfg.RateLimitWhiteList.forEach(ip => {
          const item = createListItem(ip, 'IP Address');
          rateList.appendChild(item);
        });
      
        // Populate ServerAuth entries.
        const authList = document.getElementById('serverAuthList');
        authList.innerHTML = "";
        cfg.ServerAuth.forEach(auth => {
          const item = createAuthEntry(auth);
          authList.appendChild(item);
        });
        let certType = 'selfSigned';
        if (cfg.LetsEncypt.Enabled) {
            certType = 'letsEncrypt';
        } else if (cfg.Certificate !== "" || cfg.CertificateKey !== "") {
            certType = 'ownCert';
        }
        // Populate Certificate type & fields.
        document.getElementById('certType').value = certType;
        certTypeSelect.dispatchEvent(new Event('change'));
        document.getElementById('Certificate').value = cfg.Certificate;
        document.getElementById('CertificateKey').value = cfg.CertificateKey;
        document.getElementById('LE_Domain').value = cfg.LetsEncypt.Domain;
        document.getElementById('LE_Email').value = cfg.LetsEncypt.Email;
        document.getElementById('LE_AltNames').value = cfg.LetsEncypt.AltNames.join(', ');
      
        // Populate basic Discord fields.
        document.getElementById('Discord_Client_Id').value = cfg.Discord.Client_Id;
        document.getElementById('Discord_Client_Secret').value = cfg.Discord.Client_Secret;
        document.getElementById('Discord_Bot_Token').value = cfg.Discord.Bot_Token;
        document.getElementById('Discord_Guild_Id').value = cfg.Discord.Guild_Id;
        document.getElementById('Discord_AllowToReRegister').checked = cfg.Discord.AllowToReRegister;
      
        // Populate Discord Restrictions (Advanced section).
        document.getElementById('Discord_Restrict_Sign_Up').checked = cfg.Discord.Restrict_Sign_Up;
        document.getElementById('Discord_Required_Role').value = cfg.Discord.Required_Role;
        document.getElementById('Discord_BlackList_Role').value = cfg.Discord.BlackList_Role;
        // Convert array to comma-separated string.
        document.getElementById('Restrict_Sign_Up_Countries').value = Array.isArray(cfg.Discord.Restrict_Sign_Up_Countries) 
          ? cfg.Discord.Restrict_Sign_Up_Countries.join(', ') 
          : cfg.Discord.Restrict_Sign_Up_Countries;
      
        // Populate OpenAI fields.
        document.getElementById('OpenAIApi_ApiKey').value = cfg.OpenAIApi.ApiKey;
        document.getElementById('OpenAIApi_enablePromptProtection').checked = cfg.OpenAIApi.enablePromptProtection;
      
        // Populate Functions section.
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
          div.querySelector('.delete-btn').addEventListener('click', () => { div.remove(); });
          funcContainer.appendChild(div);
        }
      }
      
      // Load config on page load.
      await loadConfig();
      
      // Save configuration on form submission.
      document.getElementById('configForm').addEventListener('submit', async function(e) {
        e.preventDefault();
        const newConfig = {
          DBServer: document.getElementById('DBServer').value,
          DB: document.getElementById('DB').value,
          CreateIndexes: document.getElementById('CreateIndexes').checked,
          IP: document.getElementById('IP').value,
          Port: Number(document.getElementById('Port').value),
          RateLimitWhiteList: Array.from(document.querySelectorAll('#rateLimitList input')).map(input => input.value),
          ServerAuth: Array.from(document.querySelectorAll('#serverAuthList .list-item')).map(item => item.querySelector('input').value),
          Certificate: document.getElementById('certType').value === 'ownCert' ? document.getElementById('Certificate').value : '',
          CertificateKey: document.getElementById('certType').value === 'ownCert' ? document.getElementById('CertificateKey').value : '',
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
      
        await window.api.saveConfig(newConfig);
      });
      
      // Cancel/Close button event: simply close the window.
      document.getElementById('cancelBtn').addEventListener('click', function() {
        window.close();
      });
              // At the very end, add an event listener for the floating save button.
              document.getElementById('floatingSaveBtn').addEventListener('click', () => {
                // You can either trigger the form's submit event or call the save logic directly.
                // Here, we'll simulate a click on the regular "Save" button:
                document.getElementById('saveBtn').click();
              });
    });
