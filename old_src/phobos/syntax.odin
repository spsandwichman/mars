package phobos

/*
┌─┐ ┌┬┐
│ │ ├┼┤
└─┘ └┴┘
┏━┓ ┏┳┓ 
┃ ┃ ┣╋┫
┗━┛ ┗┻┛

╭╮
╰╯
┌────┐ ┏━━━━━━━━━━━┓                                        ╭───╮
│file├>┨module decl┠┬───────>───────┬─┬────────>─────────┬─>│EOF│
└────┘ ┗━━━━━━━━━━━┛v               ^ v                  ^  ╰───╯
                    ├───────<───────┤ ├────────<─────────┤
                    │ ┏━━━━━━━━━━━┓ │ │ ┏━━━━━━━━━━━┓    │
                    └>┨import decl┠─┘ ├>┨declaration┠────┤
                      ┗━━━━━━━━━━━┛   │ ┗━━━━━━━━━━━┛    │
                                      │ ┏━━━━━━━━━━━━━━┓ │
                                      └>┨external block┠─┘
                                        ┗━━━━━━━━━━━━━━┛

┌───────────┐ ╭────────╮ ╭──────────╮ ╭───╮ ┌────┐
│module decl├>┤"module"├>┤identifier├>┤";"├>┤EXIT│
└───────────┘ ╰────────╯ ╰──────────╯ ╰───╯ └────┘

┌───────────┐ ╭────────╮ ╭──────────╮ ╭──────────────╮ ╭───╮ ┌────┐
│import decl├>┤"import"├>┤identifier├>┤string literal├>┤";"├>│EXIT│
└───────────┘ ╰────────╯ ╰──────────╯ ╰──────────────╯ ╰───╯ └────┘

┌───────────┐    ╭──────────╮                   ╭───╮    ┏━━━━━━━━━━┓                           ┌────┐
│declaration├─┬─>┤identifier├─┬─────────────┬─┬>┤":"├──┬>┨expression┠─┬───────────────────────┬>┤EXIT│
└───────────┘ │  ╰──────────╯ │             │ │ ╰───╯  │ ┗━━━━━━━━━━┛ v                       │ └────┘
              │               │ ┏━━━━━━━━━┓ │ │ ╭───╮  │              │   ╭───╮   ┏━━━━━━━━━━┓│
              │               └>┨directive┠─┘ └>┤","├┐ └>─────────────┴─┬>┤":"├─┬>┨expression┠┤              
              │                 ┗━━━━━━━━━┛     ╰───╯│                  │ ╰───╯ ^ ┗━━━━━━━━━━┛│
		      └────────────────<─────────────────────┘                  │ ╭───╮ │ ╭─────╮     │
		                                                                └>┤":"├─┴>┤"---"├─────┘
		                                                                  ╰───╯   ╰─────╯
┌─────────┐ ╭───╮ ╭──────────╮ 
│directive├>│"#"├>│identifier├┬
└─────────┘ ╰───╯ ╰──────────╯



















*/