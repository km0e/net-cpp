---@param name string
function set_project(name) end

---@param version string
function set_xmakever(version) end

---@param version string
---@param options table
function set_version(version, options) end

---@param rules string
---@param ... string
function add_rules(rules, ...) end

---@param warnings string
---@param ... string
function set_warnings(warnings, ...) end

---@param languages string
---@param ... string
function set_languages(languages, ...) end

---@param package string
---@param ... table | string
function add_requires(package, ...) end

---@param policy string
---@param value boolean
function set_policy(policy, value) end

---@param target string
function target(target) end

---@param kind string
function set_kind(kind) end

---@param ... string|table
function add_packages(...) end

---@param option string
function option(option) end

---@param value boolean
function set_showmenu(value) end

---@param value string | boolean
function set_default(value) end

---@param values string
---@param ... string
function set_values(values, ...) end

---@param description string
function set_description(description) end

---@param flag table | string
---@param ... table | string
function add_ldflags(flag, ...) end

---@param directory string
---@param ... string
function includes(directory, ...) end

---@param action function
function after_check(action) end

---@param config string
---@return string
function get_config(config)
    return ""
end

---@param file string
function add_configfiles(file) end

---@param name string
---@param opt table
function add_options(name, opt) end

---@param target function
function before_build(target) end

---@param target string
---@param ... table | string
function add_deps(target, ...) end

function target_end() end

---@param files string
function add_files(files) end

---@param dirs string
---@param options table?
function add_includedirs(dirs, options) end

---@param pattern string
---@param options table?
function add_headerfiles(pattern, options) end

---@param name string
---@param options table
function add_tests(name, options) end

---@param action function
function on_test(action) end

---@param name string
---@param value string
function set_config(name, value) end

---@param name string
function set_options(name) end

function option_end() end

---@param action function
function before_prepare(action) end

---@param opimization string
function set_optimize(opimization) end

---@param mode string
---@return boolean
function is_mode(mode) end

---@param name string
function rule(name) end

---@param action function
function on_load(action) end
