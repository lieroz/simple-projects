local lsp = require('lspconfig')

function OnAttach(client, bufnr)
	local opts = { noremap = true, silent = true }
	vim.api.nvim_buf_set_keymap(bufnr, 'n', '<leader>gd', '<cmd>lua vim.lsp.buf.definition()<CR>', opts)
	vim.api.nvim_buf_set_keymap(bufnr, 'n', '<leader>gi', '<cmd>lua vim.lsp.buf.implementation()<CR>', opts)

	local augroup = vim.api.nvim_create_augroup('LspFormatting', {})
	if client.supports_method("textDocument/formatting") then
		vim.api.nvim_create_autocmd('BufWritePre', {
			group = augroup,
			buffer = bufnr,
			callback = function()
				vim.lsp.buf.format()
			end,
		})
	end
end

lsp.lua_ls.setup({
	on_attach = OnAttach
})
lsp.clangd.setup({
	-- Windows doesn't support compile_commands.json generation
	on_attach = OnAttach
})
lsp.cmake.setup({
	on_attach = OnAttach
})
lsp.gdscript.setup({
	on_attach = OnAttach
})
lsp.rust_analyzer.setup({
	on_attach = OnAttach
})
