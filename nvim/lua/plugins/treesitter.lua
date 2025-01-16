require('nvim-treesitter.configs').setup({
	ensure_installed = { "c", "cpp", "cmake", "hlsl", "lua", "rust" },

	sync_install = true,
	auto_install = true,

	highlight = {
		enable = true,
		additional_vim_regex_highlighting = false,
	},
})
require('nvim-treesitter.install').compilers = { "zig" }
