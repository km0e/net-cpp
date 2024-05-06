# path


## add path to route
- `(/{segment})+` - Matche a path with one or more segments
    e.g.
    - path `/foo/bar`
        - only matche `/foo/bar`

- `(/{segment})*/` - Matche any path which has same prefix
    e.g.
    - path `/foo/`
        - matche `/foo/(.*)` , such as `/foo/bar`, `/foo/bar/baz`
        - not matche `/foo`
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
