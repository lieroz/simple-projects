vim.g.loaded_netrw = 1
vim.g.loaded_netrwPlugin = 1

vim.opt.termguicolors = true

require('nvim-tree').setup({
	actions = {
		open_file = {
			quit_on_open = true,
		},
	},
	sort = {
		sorter = 'case_sensitive',
	},
	view = {
		width = 40,
	},
	renderer = {
		group_empty = true,
	},
	filters = {
		dotfiles = false,
	},
})

vim.api.nvim_set_keymap('n', '<leader>t', '<Cmd>NvimTreeToggle<CR>', { noremap = true, silent = true })
