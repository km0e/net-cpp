---@param name string
---@return nil
function set_project(name) end

---@param version string
---@return nil
function set_xmakever(version) end

---@param version string
---@param options table
---@return nil
function set_version(version, options) end

---@param rules string
---@param ... string
---@return nil
function add_rules(rules, ...) end

---@param warnings string
---@param ... string
---@return nil
function set_warnings(warnings, ...) end

---@param languages string
---@param ... string
---@return nil
function set_languages(languages, ...) end

---@param package string
---@param ... table | string
---@return nil
function add_requires(package, ...) end

---@param policy string
---@param value boolean
---@return nil
function set_policy(policy, value) end

---@param target string
---@return nil
function target(target) end

---@param kind string
---@return nil
function set_kind(kind) end

---@param packages string
---@param options table?
---@return nil
function add_packages(packages, options) end

---@param option string
---@return nil
function option(option) end

---@param value boolean
---@return nil
function set_showmenu(value) end

---@param value string | boolean
---@return nil
function set_default(value) end

---@param values string
---@param ... string
---@return nil
function set_values(values, ...) end

---@param description string
---@return nil
function set_description(description) end

---@param flag table | string
---@param ... table | string
---@return nil
function add_ldflags(flag, ...) end

---@param directory string
---@param ... string
---@return nil
function includes(directory, ...) end

---@param action function
---@return nil
function after_check(action) end

---@param config string
---@return string
function get_config(config)
    return ""
end

---@param file string
---@return nil
function add_configfiles(file) end

---@param option string
---@return nil
function add_options(option) end

---@param target function
---@return nil
function before_build(target) end

---@param target string
---@param ... table | string
---@return nil
function add_deps(target, ...) end

---@return nil
function target_end() end

---@param files string
---@return nil
function add_files(files) end

---@param dirs string
---@param options table?
---@return nil
function add_includedirs(dirs, options) end

---@param pattern string
---@param options table?
---@return nil
function add_headerfiles(pattern, options) end

---@param name string
---@param options table
---@return nil
function add_tests(name, options) end

---@param action function
---@return nil
function on_test(action) end

---@param name string
---@param value string
---@return nil
function set_config(name, value) end

---@param name string
---@return nil
function set_options(name) end

---@return nil
function option_end() end

---@param action function
---@return nil
function before_prepare(action) end
