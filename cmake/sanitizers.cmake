
function(set_sanitizers target)
  target_compile_options(${target} PRIVATE
    -fsanitize=address
    -fsanitize=leak
    -fsanitize=undefined
    -fsanitize=pointer-subtract
    -fsanitize=pointer-compare
    )

  target_link_options(${target} PRIVATE
    -fsanitize=address
    -fsanitize=leak
    -fsanitize=undefined
    -fsanitize=pointer-subtract
    -fsanitize=pointer-compare
    )
endfunction()