<?php
require_once('FrxTemplate.inc');
class FrxTable extends FrxTemplate {
	public function generate($xml, $config) {
		$div = $this->blockDiv($config);
		$columns = $this->columns($xml);
		$table = $this->addNode($div, 4, 'table');
		$thead = $this->addNode($table, 6, 'thead');
		$throw = $this->addNode($thead, 8, 'tr');
		$tbody = $this->addNode($table, 6, 'tbody');
		$tdrow = $this->addNode($tbody, 8, 'tr', null, null, array('foreach' => '*'));
		if ($columns) foreach($columns as $col) {
			 $this->addNode($throw, 10, 'th', $col);
			 $this->addNode($tdrow, 10, 'td', '{'. $col . '}');
		}
	}
}