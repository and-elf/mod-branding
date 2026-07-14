-- mod-branding Etch one-shot branding (#31): per-item Brand state table + the etch (rank-lock) marker.
-- `item_branding` holds the #05 per-item Brand state keyed by the item's GUID counter; `etched` marks a
-- one-shot Etched item (rank-0, never upgradeable -- the core ApplyItemUpgrade refuses it). The table is
-- created here (its #05 rev was deferred). InnoDB, 4-space, trailing newline (codestyle-sql).

CREATE TABLE IF NOT EXISTS `item_branding`
(
    `item_guid`     INT UNSIGNED     NOT NULL,
    `brand`         TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `step`          TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `level_in_step` TINYINT UNSIGNED NOT NULL DEFAULT 0,
    `etched`        TINYINT UNSIGNED NOT NULL DEFAULT 0,
    PRIMARY KEY (`item_guid`)
) ENGINE = InnoDB DEFAULT CHARSET = utf8mb4 COLLATE = utf8mb4_unicode_ci;

-- The table is normally already created (without `etched`) by the earlier
-- 2026_06_21_02_item_branding.sql migration, which makes the CREATE above a no-op.
-- Add `etched` idempotently so a fresh install ends up with the column either way.
SET @has_etched := (
    SELECT COUNT(*) FROM information_schema.COLUMNS
    WHERE TABLE_SCHEMA = DATABASE()
      AND TABLE_NAME = 'item_branding'
      AND COLUMN_NAME = 'etched'
);
SET @ddl := IF(@has_etched = 0,
    'ALTER TABLE `item_branding` ADD COLUMN `etched` TINYINT UNSIGNED NOT NULL DEFAULT 0 AFTER `level_in_step`',
    'DO 0'
);
PREPARE stmt FROM @ddl;
EXECUTE stmt;
DEALLOCATE PREPARE stmt;
