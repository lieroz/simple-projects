vim.cmd [[packadd packer.nvim]]

return require('packer').startup(function(use)
	use 'wbthomason/packer.nvim'

	use 'nvim-tree/nvim-web-devicons'

	use 'lewis6991/gitsigns.nvim'

	use {
		'nvim-lualine/lualine.nvim',
		requires = { 'nvim-tree/nvim-web-devicons', opt = true }
	}

	use {
		'nvim-tree/nvim-tree.lua',
		requires = { 'nvim-tree/nvim-web-devicons', opt = true },
	}

	use {
		'romgrk/barbar.nvim',
		requires = { 'nvim-tree/nvim-web-devicons', opt = true },
		requires = { 'lewis6991/gitsigns.nvim', opt = true }
	}

	use('nvim-treesitter/nvim-treesitter', { run = ':TSUpdate' })

	use 'williamboman/mason.nvim'
	use 'williamboman/mason-lspconfig.nvim'
	use 'neovim/nvim-lspconfig'

	use 'hrsh7th/cmp-nvim-lsp'
	use 'hrsh7th/cmp-buffer'
	use 'hrsh7th/cmp-path'
	use 'hrsh7th/cmp-cmdline'
	use 'hrsh7th/nvim-cmp'

	use 'nvim-lua/plenary.nvim'

	use {
		'nvim-telescope/telescope.nvim', tag = '0.1.8',
		requires = { { 'nvim-lua/plenary.nvim' } }
	}

	use 'folke/tokyonight.nvim'

	use 'mfussenegger/nvim-dap'

	use {
		'rcarriga/nvim-dap-ui',
		requires = { 'mfussenegger/nvim-dap' },
		requires = { 'nvim-neotest/nvim-nio' }
	}

	use {
		'stevearc/overseer.nvim',
		config = function() require('overseer').setup() end
	}

	use 'mbbill/undotree'
end)
