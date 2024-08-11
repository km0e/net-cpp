# path


## add path to route
- `(/{segment})+` - Match a path with one or more segments
    e.g.
    - path `/foo/bar`
        - only match `/foo/bar`

- `(/{segment})*/` - Match any path which has same prefix
    e.g.
    - path `/foo/`
        - match `/foo/(.*)` , such as `/foo/bar`, `/foo/bar/baz`
        - not match `/foo`
## route with path
e.g.
- `/foo/bar`
    matching order:
    - `/foo/bar`
    - `/foo/`
    - `/`
- `/foo/bar/`
    matching order:
    - `/foo/bar/`
    - `/foo/bar`
    - `/foo/`
    - `/`
