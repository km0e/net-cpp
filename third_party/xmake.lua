target("cli")
do
    set_kind("phony")
    set_default(false)
    add_packages("cli11", { public = true })
end
target_end()

add_requires("quill")

target("test")
do
    set_kind("phony")
    set_default(false)
    add_packages("gtest", { public = true })
end
